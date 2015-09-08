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
// var_dump($obj);
$sql = 'select * from tags;';
print_r($obj->query($sql));


$a = $obj->getUniPri('documents');
print_r($a);
```
目前只支持实例化和query操作，日后会加以完善

2015-9-8 增加getUniPri方法，获取表的主键，返回二维数组
