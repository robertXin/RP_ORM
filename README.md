# RP_ORM
简单的orm

# Requirements
php 5.3  pdo

# Easy wear
$obj = new ormclass(array(
	'datasource'=>'mysql',
	'host' => 'localhost',
	'login' => 'xxxx',
	'password' => 'xxxxx',
	'database' => 'test',
	'port' => '3306')
);
$sql = 'select * from test;';
foreach ($obj->query($sql) as $key => $value) {
	echo $key;
	var_dump($value);
}