<?php
  session_start();

  setcookie("user","", time()-1);

  session_destroy();

  header("Location: index.php");
?>