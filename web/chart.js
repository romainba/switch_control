const AJAX_FMT = "JSON";

var width;
var date;
var module;

google.charts.load('current', {'packages':['corechart']});

Date.prototype.format = function() {
    return [this.getFullYear(), '-', this.getMonth()+1, '-', this.getDate()].join('');
};

function drawChart() {
    var next = new Date;
    next.setDate(date.getDate() + 1);

    console.log("drawchart");

    var cell = document.getElementById('date');
    cell.innerHTML = date.format();

    __drawChart('chart', null,
		date.format(),
		next.format(),
		null);
}

function __drawChart(elem, title, begin, end, hTitle) {
    var req = {
	'type'   : 'measure',
	'module' : module,
	'begin'  : begin,
	'end'    : end,
    };

    $.ajax({
	type: 'POST',
	url: 'chart.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    var options = {
		'title': title,
		'width': width,
		'height': 300,
		'hAxis': { 'title': hTitle },
		'backgroundColor': { 'fill': 'transparent' },
	    };
	    var data = new google.visualization.DataTable();
	    data.addColumn('datetime', 'date');
	    data.addColumn('number', 'temp');
	    data.addColumn('number', 'humidity');
	    data.addColumn('number', 'active');

	    response.forEach(function(e) {
	    	data.addRow([new Date(e[0]), e[1], e[2], e[3]]);
	    });
	    //var data = new google.visualization.arrayToDataTable(response);
	    var cell = document.getElementById(elem);
	    console.log(cell);
	    var chart = new google.visualization.LineChart(cell);
            chart.draw(data, options);
	},
	error: function(response) {
	    alert("ajax measure failed");
	}
    })
}

function setElem(elem, value) {
    var req = {
	'type' : elem,
	'value'  : value,
    };

    $.ajax({
	type: 'POST',
	url: 'chart.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    //console.log(response);
	    var cell = document.getElementById(elem);
	    cell.innerHTML = response;
	},
	error: function(response) {
	    alert("ajax module failed");
	}
    })
}

function prev() {
    date.setDate(date.getDate() - 1);
    drawChart();
}

function next() {
    date.setDate(date.getDate() + 1);
    drawChart();
}

function changeModule() {
    module = $("#module").val();
    console.log("module " + module);
    drawChart();
}

function toggleSwitch() {
    var req = {
	'type' : 'toggleSwitch'
    };

    $.ajax({
	type: 'POST',
	url: 'chart.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    var cell = document.getElementById('switchBtn');
	    cell.value = response;
	},
	error: function(response) {
	    alert("ajax module failed");
	}
    })
}

$(document).ready(function() {

    module = 2;
    date = new Date();
    width = $("#chart").css("width");

    setElem('moduleLst', module);
    setElem('switch');
    setElem('switchStatus');

    google.charts.setOnLoadCallback(drawChart);

    $(window).on('resize', function() {
	console.log("resize");
	var w = $("#chart").css("width");
	if (w != width) {
	    width = w;
	    drawChart();
	}
    });
})
