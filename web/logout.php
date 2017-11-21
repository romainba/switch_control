<?php
//
// logout script
//
setcookie("user","", time()-1);
session_destroy();
header("Location: index.php");
