# RP_ORM
使用pdo实现的简单的orm

# Requirements
* >php 5.3
* pdo

# Easy wear
```c
$obj = new ormclass(array(
    'datasource'=>'mysql',
    'host' => 'localhost',
    'login' => 'root',
    'password' => '123456',
    'database' => 'test',
    'port' => '3306')
);
// // var_dump($obj);
$sql = "select * from documents where id='1';";
print_r($obj->query($sql));

$sql = "select * from documents where id='1';";
print_r($obj->query($sql));


$a = $obj->getUniPri('documents');
print_r($a);

$obj->setTableName('documents');
$obj->setPriKey('id');

print_r($obj->findOne('1'));
print_r($obj->findOne('2','id'));
```

## query(string sql)
返回二维array | false

## getUniPri(string table_name)
返回二维array | false

## setTableName(string table_name)
返回bool

## setPriKey(string column)
返回bool

## findOne(string val [, string column])
返回一维数组 | false






