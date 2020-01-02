<?php

require_once('control_mqtt.php');

require_once('db_param.php');
const DB_TABLE = "lora";

$ctrl_allowed = isset($_COOKIE['user']);

$type = $_POST['type'];

switch ($type) {

case 'measure':
    $begin = $_POST['begin'];
    $end = $_POST['end'];
    $module = $_POST['module'] + 97; // convert 3 to 100

    $b = new DateTime($begin);
    $e = new DateTime($end);

    $mysql = mysql_connect('localhost', DB_USERNAME, DB_PASSWORD)
       or die('could not connect: ' . mysql_error());

    mysql_select_db(DB_NAME) or die('Could not select database');

    $query = "SELECT * FROM " . DB_TABLE .
        " WHERE date >= '" . $begin ."' AND date <= '".$end."' AND ".
        " module = " . $module .
        " ORDER BY date ASC";

    $res = mysql_query($query) or die('query failed: ' . mysql_error());

    $data = array();
    //$data[] = array('Date', 'Temp', 'vbat', 'pwr');
    $d_day = DateInterval::createFromDateString('1 day');
    $d_mount = DateInterval::createFromDateString('1 mount');

    if (($b->diff($e)) <= $d_day) {

        /* report all measurements : scatter plot */
        while ($v = mysql_fetch_array($res, MYSQL_ASSOC)) {
            $d = new DateTime($v['date']);
            $data[] = array(
                $d->format("Y/m/d H:i"), intval($v['temp'])/1000.,
                intval($v['vbat'])/1000., intval($v['pwr']));
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
            $report[] = array($key, intval($u[0])/1000.,
	    	      intval($u[1])/1000., intval($u[2]));
    }

    mysql_free_result($res);
    mysql_close($mysql);
    break;
}

echo json_encode($data);

?>