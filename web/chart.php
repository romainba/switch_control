<?php

const DB_NAME = "switch_control";
const DB_IP = "localhost";
const DB_USERNAME = "root";
const DB_PASSWORD = "abilis";
//const DB_PASSWORD = "toto1234";
const DB_TABLE = "measures";

$mysql = mysql_connect('localhost', DB_USERNAME, DB_PASSWORD)
    or die('could not connect: ' . mysql_error());

mysql_select_db(DB_NAME) or die('Could not select database');

$begin = $_POST['begin'];
$end = $_POST['end'];
$type = $_POST['type'];
$module = $_POST['module'];

$b = new DateTime($begin);
$e = new DateTime($end);

switch ($type) {
case 'temperature':

    $query = "SELECT * FROM " . DB_TABLE .
        " WHERE date >= '" . $begin ."' AND date <= '".$end."' AND ".
        " module = " . $module .
        " ORDER BY date DESC";

    $data = array();
    $r = mysql_query($query) or die('query failed: ' . mysql_error());
    while ($v = mysql_fetch_array($r, MYSQL_ASSOC)) {
        $data[] = array($v['date'], $v['temp'], $v['humidity'], $v['state']);
    }
    mysql_free_result($r);
    
    if ($b == $e) {
        $interval = DateInterval::createFromDateString('1 hour');
        $period = new DatePeriod($b, $interval, $e);

        /* merge data */
    } else {
        
        $interval = DateInterval::createFromDateString('1 month');
        $period = new DatePeriod($b, $interval, $e);
        
    }
    
    break;
}

mysql_close($mysql);

echo json_encode($data);

?>