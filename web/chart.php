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

$type = $_POST['type'];

/* modules list */
$modules = array(
    array(1, "Radiateur 1"),
    array(2, "Radiateur 2"));

switch ($type) {
case 'module':
    $data = '';
    foreach ($modules as $m) 
        $data .= '<input type="button" value="'.$m[1].'" onclick="changeModule('.$m[0].');" />';
    break;
    
case 'measure':
    $data= array();
    
    $begin = $_POST['begin'];
    $end = $_POST['end'];
    $module = $_POST['module'];

    $b = new DateTime($begin);
    $e = new DateTime($end);

    $query = "SELECT * FROM " . DB_TABLE .
        " WHERE date >= '" . $begin ."' AND date <= '".$end."' AND ".
        " module = " . $module .
        " ORDER BY date ASC";

    $res = mysql_query($query) or die('query failed: ' . mysql_error());

    $data = array();
    //$data[] = array('Date', 'Temp', 'Humidity', 'Active');
    $d_day = DateInterval::createFromDateString('1 day');
    $d_mount = DateInterval::createFromDateString('1 mount');
            
    if (($b->diff($e)) <= $d_day) {

        /* report all measurements : scatter plot */
        while ($v = mysql_fetch_array($res, MYSQL_ASSOC)) {
            $d = new DateTime($v['date']);
            $data[] = array(
                $d->format("Y/m/d H:I"), intval($v['temp']),
                intval($v['humidity']), intval($v['state']));
        }

    } else if  (($b->diff($e)) <= $d_mount) {

        /* report mean per day */
        $period = new DatePeriod($b, $d_day, $e);
        foreach($period as $dt)
            $data[$dt->format("%D")] = array(0, 0, 0);

    } else {

        /* report mean per mount */
        $period = new DatePeriod($b, $d_mount, $e);
        foreach($period as $dt)
            $data[$dt->format("%Y-%M")] = array(0, 0, 0);
        
        $report = array();
        foreach($data as $key => $u)
            $report[] = array($key, $u[0], $u[1], $u[2]);
    }

    mysql_free_result($res);
    break;
}

mysql_close($mysql);

echo json_encode($data);

?>