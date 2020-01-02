<?php

require("phpMQTT/phpMQTT.php");

$server = "192.168.1.114";
$port = 1883;
$username = "romain";
$password = "toto1234";
$client_id = "phpMQTT-23423";

const NUM_MOD = 3;
const elems = [ "heater/big", "heater/small", "room/lora" ];
const ID_LORA = 3;

const MQTT_STATE_TOPIC      = "%s/state";
const MQTT_STATE_SET_TOPIC  = "%s/state_set";
const MQTT_TARGET_TOPIC     = "%s/target";
const MQTT_TARGET_SET_TOPIC = "%s/target_set";
const MQTT_TEMP_TOPIC       = "%s/temp";
const MQTT_MEASURES_TOPIC   = "%s/measures";

$received_topic = null;
$received_name = null;

function topic($topic, $module)
{
	if ($module < 1 || $module > NUM_MOD)
	   return "";

	return sprintf($topic, elems[$module - 1]);
}

function mqtt_connect()
{
    global $server, $port, $client_id, $username, $password;

    $mqtt = new Bluerhinos\phpMQTT($server, $port, $client_id);

    if (!$mqtt->connect(true, NULL, $username, $password)) {
        echo "connect timeout\n";
        return;
    }
    return $mqtt;
}

function procmsg($topic, $msg)
{
     global $received_topic, $received_msg;

     $received_topic = $topic;
     $received_msg = $msg;
}

function getMqttValue($name)
{
    global $received_topic, $received_msg;

    $mqtt = mqtt_connect();

    $topics[$name] = array(
        "qos" => 0,
        "function" => "procmsg"
    );

    $mqtt->subscribe($topics, 0);
    while ($mqtt->proc());
    $mqtt->close();

    return $received_msg;
}

function setMqttValue($name, $value)
{
    $mqtt = mqtt_connect();
    $mqtt->publish($name, $value, 0);
    $mqtt->close();
}

function getState($module)
{
    if ($module >= ID_LORA)
        return getMqttValue(topic(MQTT_MEASURES_TOPIC, $module));
    else
	return getMqttValue(topic(MQTT_STATE_TOPIC, $module));
}

function setState($module, $value)
{
    setMqttValue(topic(MQTT_STATE_SET_TOPIC, $module),
                 $value);
}

function getTemp($module)
{
    return getMqttValue(topic(MQTT_TEMP_TOPIC, $module));
}

function toggleState($module)
{
    global $state;

    $s = getState($module);
    if ($s == 'ON')
       $new = 'OFF';
    else
       $new = 'ON';

    setState($module, $new);
    $state[$module - 1] = $new;
    return $new;
}