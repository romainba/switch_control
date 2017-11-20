<?php

const HOSTNAME = '10.41.22.211';

const CMD_SET_SW_POS = 0;
const CMD_TOGGLE_MODE = 1;
const CMD_SET_TEMP = 2;
const CMD_GET_STATUS = 3;

$service_port = 8998;

function getStatus($port)
{
    $address = gethostbyname(HOSTNAME);

    $s = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($s === false) {
        echo "socket_create() a échoué : raison :  " . socket_strerror(socket_last_error()) . "\n";
        return;
    }

    $result = socket_connect($s, $address, $port);
    if ($s === false) {
        echo "socket_connect() a échoué : raison : ($result) " . socket_strerror(socket_last_error($s)) . "\n";
        return;
    }

    $buf = array(CMD_GET_STATUS, 0 /* size */);

    $str = '';
    foreach ($buf as $b)
        $str .= pack("L", $b);

    socket_write($s, $str, sizeof($buf));
   
    $out = socket_read($s, 2048);
    $buf = unpack("L*", $out);

    socket_close($s);

    return array($buf[4] /* threshold */, $buf[5] /* sw_pos */, $buf[6] /* temp */);
}

function toggleSwitch($port)
{
    $address = gethostbyname(HOSTNAME);

    $s = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($s === false) {
        echo "socket_create() a échoué : raison :  " . socket_strerror(socket_last_error()) . "\n";
        return;
    }

    $result = socket_connect($s, $address, $port);
    if ($s === false) {
        echo "socket_connect() a échoué : raison : ($result) " . socket_strerror(socket_last_error($s)) . "\n";
        return;
    }

    $buf = array(CMD_TOGGLE_MODE, 0 /* size */);

    $str = '';
    foreach ($buf as $b)
        $str .= pack("L", $b);

    socket_write($s, $str, sizeof($buf));

    socket_close($s);
}
