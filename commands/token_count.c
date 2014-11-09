/* Copyright(C) 2014 Naoya Murakami <naoya@createfield.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301  USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <groonga/plugin.h>
#include <groonga/nfkc.h>

#ifdef __GNUC__
# define GNUC_UNUSED __attribute__((__unused__))
#else
# define GNUC_UNUSED
#endif

static grn_obj *
command_token_count(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *var;
  char *table_name = NULL;
  int table_name_length = -1;
  char *index_column_name = NULL;
  int index_column_name_length = -1;
  int token_size = -1;
  char *char_type = NULL;
  int char_type_length = -1;
  char *sortby = (char *)"-_value";
  unsigned int sortby_length = 7;
  int limit = -1;
  float ratio = -1; /* 0.01 */
  int threshold = -1; /* 10000000 */
  grn_bool use_ctx_output = GRN_FALSE;

  var = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    table_name = GRN_TEXT_VALUE(var);
    table_name_length = GRN_TEXT_LEN(var);
  } else {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "[token_count] table name is missing");
    return NULL;
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "column", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    index_column_name = GRN_TEXT_VALUE(var);
    index_column_name_length = GRN_TEXT_LEN(var);
  } else {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "[token_count] index column name is missing");
    return NULL;
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "token_size", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    token_size = atoi(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "ctype", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    char_type = GRN_TEXT_VALUE(var);
    char_type_length = GRN_TEXT_LEN(var);
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "sortby", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    sortby = GRN_TEXT_VALUE(var);
    sortby_length = GRN_TEXT_LEN(var);
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "limit", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    limit = atoi(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "ratio", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    ratio = atof(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "threshold", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    threshold = atoi(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "use_ctx_output", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    use_ctx_output = atoi(GRN_TEXT_VALUE(var));
  }

  grn_obj *table;
  grn_obj *index_column;
  grn_obj *hash;
  grn_obj *sorted;
  unsigned int n_terms;
  unsigned long total = 0;

  table = grn_ctx_get(ctx, table_name, table_name_length);
  if (!table) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[token_count] table doesn't exist: <%.*s>",
                     table_name_length, table_name);
    return NULL;
  }

  index_column = grn_obj_column(ctx, table,
                                index_column_name,
                                index_column_name_length);

  if (!(index_column->header.flags & GRN_OBJ_COLUMN_INDEX)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[token_count] column isn't COLUMN_INDEX: <%.*s.%.*s>",
                     table_name_length, table_name,
                     index_column_name_length, index_column_name);
    return NULL;
  }
  hash = grn_table_create(ctx, NULL, 0, NULL,
                          GRN_OBJ_TABLE_HASH_KEY,
                          grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                          grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!hash) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[token_count] couldn't create a hash");
    return NULL;
  }

  {
    grn_table_cursor *cur;
    if ((cur = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1,
                                     GRN_CURSOR_BY_ID))) {

      grn_obj *index_cursor;
      if ((index_cursor = grn_index_cursor_open(ctx, cur, index_column,
                                                0, -1, GRN_CURSOR_BY_ID))) {
        grn_posting *posting;
        grn_id term_id;
        char term[GRN_TABLE_MAX_KEY_SIZE];
        int term_length = 0;
        grn_obj value;
        GRN_UINT32_INIT(&value, 0);

        while ((posting = grn_index_cursor_next(ctx, index_cursor, &term_id))) {
          grn_char_type type;
          grn_encoding encoding = ctx->encoding;
          grn_bool is_add = GRN_FALSE;
          term_length = grn_table_get_key(ctx, table,
                                          term_id, term,
                                          GRN_TABLE_MAX_KEY_SIZE);
          type = grn_nfkc_char_type((unsigned char *)term);
          if (char_type_length == 5 &&
              !memcmp("alpha", char_type, 5)) {
            if (type == GRN_CHAR_ALPHA) {
              if (token_size == term_length || token_size < 0) {
                is_add = GRN_TRUE;
              }
            }
          } else {
            int char_length;
            int rest_length = term_length;
            const char *rest = (const char *)term;
            int term_size = 0;

            while (rest_length > 0) {
              char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
              if (char_length == 0) {
                break;
              }
              term_size++;
              rest += char_length;
              rest_length -= char_length;
            }
            if (term_size == token_size || token_size < 0) {
              if(char_type_length == 2 && !memcmp("ja", char_type, 2)) {
                if(type == GRN_CHAR_HIRAGANA || type == GRN_CHAR_KATAKANA ||
                   type == GRN_CHAR_KANJI || type == GRN_CHAR_OTHERS) {
                  is_add = GRN_TRUE;
                }
              } else {
                is_add = GRN_TRUE;
              }
            }
          }
          if (is_add) {
            grn_id hash_id;
            hash_id = grn_table_add(ctx, hash, term, term_length, NULL);
            if (hash_id) {
              GRN_BULK_REWIND(&value);
              grn_obj_get_value(ctx, hash, hash_id, &value);
              GRN_UINT32_SET(ctx, &value, GRN_UINT32_VALUE(&value) + posting->tf);
              grn_obj_set_value(ctx, hash, hash_id, &value, GRN_OBJ_SET);
            }
            total += posting->tf;
          }
        }
        grn_obj_unlink(ctx, &value);
      }
      grn_obj_unlink(ctx, index_cursor);
    }
    grn_table_cursor_close(ctx, cur);
  }
  n_terms = grn_table_size(ctx, hash);
  if (ratio > 0 && ratio < 1) {
    limit = n_terms * ratio;
  }
  if (limit < 0) {
    limit = n_terms;
  }

  {
    unsigned int nkeys;
    grn_table_sort_key *keys;
    int offset = 0;
    sorted = grn_table_create(ctx, NULL, 0, NULL,
                              GRN_OBJ_TABLE_NO_KEY, NULL, hash);
    if (!sorted) {
      GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                       "[token_count] couldn't create a sort table");
      return NULL;
    }

    keys = grn_table_sort_key_from_str(ctx, sortby, sortby_length, hash, &nkeys);
    if (keys) {
      grn_table_sort(ctx, hash, offset, -1, sorted, keys, nkeys);
      grn_table_sort_key_close(ctx, keys, nkeys);
    }
  }

  {
    grn_table_cursor *cur;
    if ((cur = grn_table_cursor_open(ctx, sorted, NULL, 0, NULL,
                                     0, 0, limit, GRN_CURSOR_BY_ID))) {
      grn_id hash_id;
      grn_obj value;
      GRN_UINT32_INIT(&value, 0);
      if (use_ctx_output) {
        grn_ctx_output_array_open(ctx, "TOKENS", limit);
        grn_ctx_output_array_open(ctx, "TOTAL", 2);
        grn_ctx_output_int64(ctx, total);
        grn_ctx_output_int32(ctx, n_terms);
        grn_ctx_output_array_close(ctx);
      }
      while ((hash_id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
        unsigned int sorted_key;
        grn_table_get_key(ctx, sorted, hash_id, &sorted_key, sizeof(unsigned int));
        {
          char key[GRN_TABLE_MAX_KEY_SIZE];
          int key_size;
          key_size = grn_table_get_key(ctx, hash, sorted_key, &key, GRN_TABLE_MAX_KEY_SIZE);

          GRN_BULK_REWIND(&value);
          grn_obj_get_value(ctx, hash, sorted_key, &value);
          if (GRN_UINT32_VALUE(&value) >= (unsigned int)threshold || threshold < 0) {
            if (use_ctx_output) {
              grn_ctx_output_array_open(ctx, "TOKEN", 2);
              grn_ctx_output_str(ctx, key, key_size);
              grn_ctx_output_int32(ctx, GRN_UINT32_VALUE(&value));
              grn_ctx_output_array_close(ctx);
            } else {
              printf("%.*s,%d\n", key_size, key, GRN_UINT32_VALUE(&value));
            }
          }
        }
      }
      if (use_ctx_output) {
        grn_ctx_output_array_close(ctx);
      } else {
        printf("Total tokens = %lu\n", total);
        printf("Total kinds = %d\n", n_terms);

      }
      grn_obj_unlink(ctx, &value);
    }
    grn_table_cursor_close(ctx, cur);
  }

  grn_obj_unlink(ctx, hash);
  grn_obj_unlink(ctx, index_column);
  grn_obj_unlink(ctx, table);
  grn_obj_unlink(ctx, sorted);

  return NULL;
}

static grn_obj *
command_document_count(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
                       grn_user_data *user_data)
{
  grn_obj *var;
  char *table_name = NULL;
  int table_name_length = -1;
  char *index_column_name = NULL;
  int index_column_name_length = -1;
  int token_size = -1;
  char *char_type = NULL;
  int char_type_length = -1;
  char *sortby = (char *)"-_value";
  unsigned int sortby_length = 7;
  int limit = -1;
  float ratio = -1; /* 0.01 */
  int threshold = -1; /* 10000000 */
  grn_bool use_ctx_output = GRN_FALSE;

  var = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    table_name = GRN_TEXT_VALUE(var);
    table_name_length = GRN_TEXT_LEN(var);
  } else {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "[document_count] table name is missing");
    return NULL;
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "column", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    index_column_name = GRN_TEXT_VALUE(var);
    index_column_name_length = GRN_TEXT_LEN(var);
  } else {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "[document_count] index column name is missing");
    return NULL;
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "token_size", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    token_size = atoi(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "ctype", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    char_type = GRN_TEXT_VALUE(var);
    char_type_length = GRN_TEXT_LEN(var);
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "sortby", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    sortby = GRN_TEXT_VALUE(var);
    sortby_length = GRN_TEXT_LEN(var);
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "limit", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    limit = atoi(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "ratio", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    ratio = atof(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "threshold", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    threshold = atoi(GRN_TEXT_VALUE(var));
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "use_ctx_output", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    use_ctx_output = atoi(GRN_TEXT_VALUE(var));
  }

  grn_obj *table;
  grn_obj *index_column;
  grn_obj *hash;
  grn_obj *sorted;
  unsigned int n_terms;
  unsigned int n_documents = 0;
  unsigned long total = 0;

  table = grn_ctx_get(ctx, table_name, table_name_length);
  if (!table) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[document_count] table doesn't exist: <%.*s>",
                     table_name_length, table_name);
    return NULL;
  }

  index_column = grn_obj_column(ctx, table,
                                index_column_name,
                                index_column_name_length);

  if (!(index_column->header.flags & GRN_OBJ_COLUMN_INDEX)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[document_count] column isn't COLUMN_INDEX: <%.*s.%.*s>",
                     table_name_length, table_name,
                     index_column_name_length, index_column_name);
    return NULL;
  }

  {
    grn_obj source_ids;
    unsigned int i, n_ids;
    grn_obj *source_table;

    GRN_UINT32_INIT(&source_ids, GRN_OBJ_VECTOR);
    grn_obj_get_info(ctx, index_column, GRN_INFO_SOURCE, &source_ids);
    n_ids = GRN_BULK_VSIZE(&source_ids) / sizeof(grn_id);

    for (i = 0; i < n_ids; i++) {
      grn_id source_id;
      grn_obj *source;
      source_id = GRN_UINT32_VALUE_AT(&source_ids, i);
      source = grn_ctx_at(ctx, source_id);
      source_table = grn_column_table(ctx, source);
      n_documents += grn_table_size(ctx, source_table);
    }
    grn_obj_unlink(ctx, &source_ids);
  }

  hash = grn_table_create(ctx, NULL, 0, NULL,
                          GRN_OBJ_TABLE_HASH_KEY,
                          grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                          grn_ctx_at(ctx, GRN_DB_UINT32));
  if (!hash) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[document_count] couldn't create a hash");
    return NULL;
  }

  {
    grn_table_cursor *cur;
    if ((cur = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1,
                                     GRN_CURSOR_BY_ID))) {
      grn_id id;
      grn_obj df;
      GRN_UINT32_INIT(&df, 0);
      while ((id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
        char term[GRN_TABLE_MAX_KEY_SIZE];
        int term_length = 0;

        grn_char_type type;
        grn_encoding encoding = ctx->encoding;
        grn_bool is_add = GRN_FALSE;

        term_length = grn_table_get_key(ctx, table, id, term,
                                        GRN_TABLE_MAX_KEY_SIZE);
        GRN_BULK_REWIND(&df);
        grn_obj_get_value(ctx, index_column, id, &df);

        type = grn_nfkc_char_type((unsigned char *)term);
        if (char_type_length == 5 &&
            !memcmp("alpha", char_type, 5)) {
          if (type == GRN_CHAR_ALPHA) {
            if (token_size == term_length || token_size < 0) {
              is_add = GRN_TRUE;
            }
          }
        } else {
          int char_length;
          int rest_length = term_length;
          const char *rest = (const char *)term;
          int term_size = 0;

          while (rest_length > 0) {
            char_length = grn_plugin_charlen(ctx, rest, rest_length, encoding);
            if (char_length == 0) {
              break;
            }
            term_size++;
            rest += char_length;
            rest_length -= char_length;
          }
          if (term_size == token_size || token_size < 0) {
            if(char_type_length == 2 && !memcmp("ja", char_type, 2)) {
              if(type == GRN_CHAR_HIRAGANA || type == GRN_CHAR_KATAKANA ||
                 type == GRN_CHAR_KANJI || type == GRN_CHAR_OTHERS) {
                is_add = GRN_TRUE;
              }
            } else {
              is_add = GRN_TRUE;
            }
          }
        }

        if (is_add) {
          grn_id hash_id;
          hash_id = grn_table_add(ctx, hash, term, term_length, NULL);
          if (hash_id) {
            grn_obj_set_value(ctx, hash, hash_id, &df, GRN_OBJ_SET);
          }
          total += GRN_UINT32_VALUE(&df);
        }
        grn_obj_unlink(ctx, &df);
      }
    }
    grn_table_cursor_close(ctx, cur);
  }
  n_terms = grn_table_size(ctx, hash);
  if (limit < 0) {
    limit = n_terms;
  }

  {
    unsigned int nkeys;
    grn_table_sort_key *keys;
    int offset = 0;
    sorted = grn_table_create(ctx, NULL, 0, NULL,
                              GRN_OBJ_TABLE_NO_KEY, NULL, hash);
    if (!sorted) {
      GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                       "[token_count] couldn't create a sort table");
      return NULL;
    }

    keys = grn_table_sort_key_from_str(ctx, sortby, sortby_length, hash, &nkeys);
    if (keys) {
      grn_table_sort(ctx, hash, offset, -1, sorted, keys, nkeys);
      grn_table_sort_key_close(ctx, keys, nkeys);
    }
  }

  {
    grn_table_cursor *cur;
    if ((cur = grn_table_cursor_open(ctx, sorted, NULL, 0, NULL,
                                     0, 0, limit, GRN_CURSOR_BY_ID))) {
      grn_id hash_id;
      grn_obj value;
      GRN_UINT32_INIT(&value, 0);
      if (use_ctx_output) {
        grn_ctx_output_array_open(ctx, "TOKENS", limit);
        grn_ctx_output_array_open(ctx, "TOTAL", 3);
        grn_ctx_output_int64(ctx, total);
        grn_ctx_output_int32(ctx, n_documents);
        grn_ctx_output_int32(ctx, n_terms);
        grn_ctx_output_array_close(ctx);
      }
      while ((hash_id = grn_table_cursor_next(ctx, cur)) != GRN_ID_NIL) {
        unsigned int sorted_key;
        grn_table_get_key(ctx, sorted, hash_id, &sorted_key, sizeof(unsigned int));
        {
          char key[GRN_TABLE_MAX_KEY_SIZE];
          int key_size;
          key_size = grn_table_get_key(ctx, hash, sorted_key, &key, GRN_TABLE_MAX_KEY_SIZE);

          GRN_BULK_REWIND(&value);
          grn_obj_get_value(ctx, hash, sorted_key, &value);
          if (GRN_UINT32_VALUE(&value) >= (unsigned int)threshold || threshold < 0) {
            float idf = (float)GRN_UINT32_VALUE(&value) / (float)n_documents;
            if (idf > ratio || ratio < 0) {
              if (use_ctx_output) {
                grn_ctx_output_array_open(ctx, "TOKEN", 3);
                grn_ctx_output_str(ctx, key, key_size);
                grn_ctx_output_int32(ctx, GRN_UINT32_VALUE(&value));
                grn_ctx_output_float(ctx, idf);
                grn_ctx_output_array_close(ctx);
              } else {
                printf("%.*s,%d,%f\n",
                       key_size, key,
                       GRN_UINT32_VALUE(&value),
                       idf);
              }
            }
          }
        }
      }
      if (use_ctx_output) {
        grn_ctx_output_array_close(ctx);
      } else {
        printf("Total tokens(include tolerance) = %lu\n", total);
        printf("Total documents = %d\n", n_documents);
        printf("Total kinds of token = %d\n", n_terms);
      }
      grn_obj_unlink(ctx, &value);
    }
    grn_table_cursor_close(ctx, cur);
  }

  grn_obj_unlink(ctx, hash);
  grn_obj_unlink(ctx, index_column);
  grn_obj_unlink(ctx, table);
  grn_obj_unlink(ctx, sorted);

  return NULL;
}

grn_rc
GRN_PLUGIN_INIT(GNUC_UNUSED grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_expr_var vars[9];

  grn_plugin_expr_var_init(ctx, &vars[0], "table", -1);
  grn_plugin_expr_var_init(ctx, &vars[1], "column", -1);
  grn_plugin_expr_var_init(ctx, &vars[2], "token_size", -1);
  grn_plugin_expr_var_init(ctx, &vars[3], "ctype", -1);
  grn_plugin_expr_var_init(ctx, &vars[4], "sortby", -1);
  grn_plugin_expr_var_init(ctx, &vars[5], "limit", -1);
  grn_plugin_expr_var_init(ctx, &vars[6], "ratio", -1);
  grn_plugin_expr_var_init(ctx, &vars[7], "threshold", -1);
  grn_plugin_expr_var_init(ctx, &vars[8], "use_ctx_output", -1);
  grn_plugin_command_create(ctx, "token_count", -1, command_token_count, 9, vars);

  grn_plugin_command_create(ctx, "document_count", -1, command_document_count, 9, vars);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(GNUC_UNUSED grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
