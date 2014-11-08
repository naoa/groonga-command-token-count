# Groonga token count command plugin

## Command

* ``token_count``

指定したインデックスカラムからトークンの出現数をカウントします。

* 実行例

```
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

* 出力形式  
標準出力に``トークン,出現数``のcsv形式が出力されます。  
``-- use_ctx_output 1``を利用した場合、json形式で出力されます。

* 実行時間例

161.192GiBのTokenBigramの日本語文書での実行時間

```
real    19m18.369s
user    17m10.271s
sys     1m3.834s
```

## Install

Install ``groonga-command-token_count`` package:

### CentOS

* CentOS6

```
% sudo yum localinstall -y http://packages.createfield.com/centos/6/groonga-command-token-count-0.0.1-1.el6.x86_64.rpm
```

* CentOS7

```
% sudo yum localinstall -y http://packages.createfield.com/centos/7/groonga-command-token-count-0.0.1-1.el7.centos.x86_64.rpm
```

### Fedora

* Fedora 20

```
% sudo yum localinstall -y http://packages.createfield.com/fedora/20/groonga-command-token-count-0.0.1-1.fc20.x86_64.rpm
```

* Fedora 21

```
% sudo yum localinstall -y http://packages.createfield.com/fedora/21/groonga-command-token-count-0.0.1-1.fc21.x86_64.rpm
```

### Debian GNU/LINUX

* wheezy

```
% wget http://packages.createfield.com/debian/wheezy/groonga-command-token-count_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.1-1_amd64.deb
```

* jessie

```
% wget http://packages.createfield.com/debian/jessie/groonga-command-token-count_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.1-1_amd64.deb
```

### Ubuntu

* precise

```
% wget http://packages.createfield.com/ubuntu/precise/groonga-command-token-count_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.1-1_amd64.deb
```

* trusty

```
% wget http://packages.createfield.com/ubuntu/trusty/groonga-command-token-count_0.0.1-1_amd64.deb
% sudo dpkg -i groonga-command-token-count_0.0.1-1_amd64.deb
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
