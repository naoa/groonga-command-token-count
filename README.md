# Groonga token count command plugin

## Command

### ``token_count``

指定したインデックスカラムからトークンの出現数をカウントします。

* 実行例

```bash
token_count Terms document_index --token_size 2 --limit 3 --use_ctx_output 1
[[0,0.0,0.0],[[23,10],["今日",5],["日は",5],["は雨",3]]]

token_count Terms document_index --token_size 2 --limit 3
今日,5
日は,5
は雨,3
Total tokens = 23
Total kinds = 10
```

* 入力形式

| arg        | description |default|
|:-----------|:------------|:------|
| table      | テーブルを指定 | NULL |
| column     | インデックスカラムを指定 | NULL |
| token_size | 出力トークンの文字数を指定 | -1 |
| ctype | 出力トークンの文字種を指定(ja/alpha) | all |
| sortby | 出力トークンのソートを指定(_key/_value) | ``-_value`` |
| limit | 出力トークンの数を指定 | -1 |
| ratio | 出力トークンの数を全体トークン数に対する比率で指定 | -1 |
| threshold | 出力トークンの最低出現数を指定 | -1 |
| use_ctx_output | json形式で出力するかを指定(0/1) | 0 |
| output_prefix | 先頭1文字を別に出力するかを指定(0/1) | 0 |

* 出力形式  
標準出力に``トークン,出現数``のcsv形式が出力されます。  
``-- use_ctx_output 1``を利用した場合、json形式で出力されます。

* 実行時間例

約80GiBのTokenBigramの日本語文書での実行時間

```bash
real    19m18.369s
user    17m10.271s
sys     1m3.834s
```

### ``document_count``

指定したインデックスカラムからトークンが含まれるドキュメント数DF(Document Frequency)をカウントします。DF値が大きいものはどのドキュメントにもそのトークンが現れていることを示しており、トークンの重要度を示す尺度として利用されることがあります。以下の例では、``a``と``b``は全ての文書に含まれているため、値が大きく、``e``は1つの文書にしか含まれていないため、値が小さくなっています。

* 実行例

```bash
table_create Entries TABLE_NO_KEY
column_create Entries body COLUMN_SCALAR ShortText
table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
load --table Entries
[
{"body": "a b c d e"},
{"body": "a b c d"},
{"body": "a b c"},
{"body": "a b"}
]
column_create Terms document_index COLUMN_INDEX|WITH_POSITION Entries body
document_count Terms document_index --token_size -1
a,6,1.500000
b,6,1.500000
c,5,1.250000
d,4,1.000000
e,1,0.250000
Total tokens(include tolerance) = 22
Total documents = 4
Total kinds of token = 5
```

* 入力形式

| arg        | description |default|
|:-----------|:------------|:------|
| table      | テーブルを指定 | NULL |
| column     | インデックスカラムを指定 | NULL |
| token_size | 出力トークンの文字数を指定 | -1 |
| ctype | 出力トークンの文字種を指定(ja/alpha) | all |
| sortby | 出力トークンのソートを指定(_key/_value) | ``-_value`` |
| limit | 出力トークンの数を指定 | -1 |
| ratio | 出力トークンの数を全体ドキュメント数に対する比率で指定 | -1 |
| threshold | 出力トークンの最低出現数を指定 | -1 |
| use_ctx_output | json形式で出力するかを指定(0/1) | 0 |

* 出力形式  
標準出力に``トークン,出現ドキュメント数(DF),出現ドキュメント数/全ドキュメント数(IDFではない)``のcsv形式が出力されます。
``-- use_ctx_output 1``を利用した場合、json形式で出力されます。配列の一つ目は、``総トークン数,総ドキュメント数,トークンの種類数``です。

なお、GroongaのDF値には誤差が含まれているため、比が1以上の値になることがあります。静的索引構築の場合、DF値の誤差は少ないですが(たぶん2多いだけ)、動的索引構築の場合、ドキュメントに含まれるトークンの種類ごとにDF値が1ずつ増える気がします。ただ、トークン数が十分に多い文書を多数読み込ませれば、問題ない程度でしょう。

* 実行時間例

約80GiBのTokenBigramの日本語文書での実行時間

```bash
real    0m2.763s
user    0m2.627s
sys     0.0.234s
```

## Install

Install ``groonga-command-token-count`` package:

### CentOS

* CentOS6

```
% sudo yum localinstall -y http://packages.createfield.com/centos/6/groonga-command-token-count-0.0.2-1.el6.x86_64.rpm
```

* CentOS7

```
% sudo yum localinstall -y http://packages.createfield.com/centos/7/groonga-command-token-count-0.0.2-1.el7.centos.x86_64.rpm
```

### Fedora

* Fedora 20

```
% sudo yum localinstall -y http://packages.createfield.com/fedora/20/groonga-command-token-count-0.0.2-1.fc20.x86_64.rpm
```

* Fedora 21

```
% sudo yum localinstall -y http://packages.createfield.com/fedora/21/groonga-command-token-count-0.0.2-1.fc21.x86_64.rpm
```

### Debian GNU/LINUX

* wheezy

```
% wget http://packages.createfield.com/debian/wheezy/groonga-command-token-count_0.0.2-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.2-1_amd64.deb
```

* jessie

```
% wget http://packages.createfield.com/debian/jessie/groonga-command-token-count_0.0.2-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.2-1_amd64.deb
```

* sid

```
% wget http://packages.createfield.com/debian/sid/groonga-command-token-count_0.0.2-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.2-1_amd64.deb
```

### Ubuntu

* precise

```
% wget http://packages.createfield.com/ubuntu/precise/groonga-command-token-count_0.0.2-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.2-1_amd64.deb
```

* trusty

```
% wget http://packages.createfield.com/ubuntu/trusty/groonga-command-token-count_0.0.2-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.2-1_amd64.deb
```
* utopic

```
% wget http://packages.createfield.com/ubuntu/utopic/groonga-command-token-count_0.0.2-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.2-1_amd64.deb
```

### Source install

Build this normalizer.

    % sh autogen.sh
    % ./configure
    % make
    % sudo make install

## Dependencies

* Groonga >= 4.0.0

Install ``groonga-devel`` in CentOS/Fedora.
Install ``libgroonga-dev`` in Debian/Ubuntu.  

See http://groonga.org/docs/install.html

## Usage

Firstly, register `commands/token_count`

Groonga:

    % groonga db
    > register commands/token_count
    > token_count Terms index

Mroonga:

    mysql> use db;
    mysql> select mroonga_command("register commands/token_count");
    mysql> select mroonga_command("token_count diaries-content index");

## Author

* Naoya Murakami <naoya@createfield.com>

## License

LGPL 2.1. See COPYING for details.

This program is the same license as Groonga.
