<?php

require_once('control_mqtt.php');

$ctrl_allowed = isset($_COOKIE['user']);

const DB_NAME = "switch_control";
const DB_IP = "localhost";
const DB_USERNAME = "root";
//const DB_PASSWORD = "abilis";
const DB_PASSWORD = "toto1234";
const DB_TABLE = "measures";

if ($argv) {
   for ($i = 1; $i < sizeof($argv); $i++) {
     $g = explode('=', $argv[$i]);
     $_POST[$g[0]] = $g[1];
   }
}

$type = $_POST['type'];
$module = $_POST['module'];

/* request type */
switch ($type) {

case 'switchStatus':

  if (!$ctrl_allowed) {
    $data = "state " . getState($module);
  } else {
    $data = 'state <input id="switchBtn' . $module . '"  type="button"';
    $data .= ' value=' . getState($module);

    $t = 'toggleState(' . $module . ')';
    $data .= ' onclick="toggleState(' . $module . ');" />';
  }
  $data .= " temp " . getTemp($module);
  break;

case 'toggleState':
  if ($ctrl_allowed)
    $data = toggleState($module);
  else
    $data = 'not allowed';
  break;

case 'measure':
    $begin = $_POST['begin'];
    $end = $_POST['end'];

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
    //$data[] = array('Date', 'Temp', 'Humidity', 'Active');
    $d_day = DateInterval::createFromDateString('1 day');
    $d_mount = DateInterval::createFromDateString('1 mount');

    if (($b->diff($e)) <= $d_day) {

        /* report all measurements : scatter plot */
        while ($v = mysql_fetch_array($res, MYSQL_ASSOC)) {
            $d = new DateTime($v['date']);
            $data[] = array(
                $d->format("Y/m/d H:i"), intval($v['temp'])/1000.,
                intval($v['humidity'])/100., intval($v['state']));
        }

    } else if  (($b->diff($e)) <= $d_mount) {

        /* report mean per day */
        $period = new DatePeriod($b, $d_day, $e);
        foreach($period as $dt)
            $data[$dt->format("%D")] = array(0, 0, 0);

    } else {

        /* report mean per month */
        $period = new DatePeriod($b, $d_mount, $e);
        foreach($period as $dt)
            $data[$dt->format("%Y-%M")] = array(0, 0, 0);

        $report = array();
        foreach($data as $key => $u)
            $report[] = array($key, intval($u[0])/1000.,
	    	      intval($u[1])/100., $u[2]);
    }

    mysql_free_result($res);
    mysql_close($mysql);
    break;
default:
    $data = 'unknown request';
}

echo json_encode($data);

?>