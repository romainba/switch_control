var width;
var date;

const AJAX_FMT = "JSON";

const ID_HEATER_BIG = 1;
const ID_HEATER_SMALL = 2;
const ID_LORA = 3;

google.charts.load('current', {'packages':['corechart']});

Date.prototype.format = function() {
    return [this.getFullYear(), '-', this.getMonth()+1, '-', this.getDate()].join('');
};

function drawCharts()
{
    var next = new Date;
    next.setDate(date.getDate() + 1);
    //console.log("drawchart");

    var cell = document.getElementById('date');
    cell.innerHTML = date.format();

    __drawChart('chart', ID_HEATER_BIG, null,
		date.format(), next.format(), null);
    __drawChart('chart', ID_HEATER_SMALL, null,
		date.format(), next.format(), null);
    __drawChart_lora('chart', ID_LORA, null,
		     date.format(), next.format(), null);
}

function __drawChart(elem, module, title, begin, end, hTitle)
{
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
		title: title,
		width: width,
		height: 300,
		hAxis: { title: hTitle },
		backgroundColor: { fill: 'transparent' },
		series: {
		    0: { targetAxisIndex: 0 },
		    1: { targetAxisIndex: 1, type: 'steppedArea' }
		},
		vAxes: {
		    0: { viewWindowMode:'explicit',
			 viewWindow: {
			     max:0,
			     min:30
			 }
		       },
		    1: { viewWindowMode:'explicit',
			 viewWindow: {
			     max:0,
			     min:5
			 },
			 gridlines: { count: 0 },
		       }
		},
	    };
	    var data = new google.visualization.DataTable();
	    data.addColumn('datetime', 'date');
	    data.addColumn('number', 'temperature');
	    //data.addColumn('number', 'humidity');
	    data.addColumn('number', 'activation');

	    response.forEach(function(e) {
		data.addRow([new Date(e[0]), e[1], /* e[2], */ e[3]]);
	    });
	    //var data = new google.visualization.arrayToDataTable(response);
	    var cell = document.getElementById(elem + module);
	    //console.log(cell);
	    var chart = new google.visualization.LineChart(cell);
            chart.draw(data, options);
	},
	error: function(response) {
	    alert("ajax measure failed");
	}
    })
}

function setElem(elem, module)
{
    var req = {
	'type' : elem,
	'module' : module,
    };
    var name = elem + module

    $.ajax({
	type: 'POST',
	url: 'chart.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    //console.log(response);
	    var name = elem + module
	    var cell = document.getElementById(name);
	    cell.innerHTML = response;
	},
	error: function(response) {
	    alert("ajax module failed");
	}
    })
}

function __drawChart_lora(elem, module, title, begin, end, hTitle)
{
    var req = {
	'type'   : 'measure',
	'module' : module,
	'begin'  : begin,
	'end'    : end,
    };

    $.ajax({
	type: 'POST',
	url: 'chart_lora.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    var options = {
		title: title,
		width: width,
		height: 300,
		hAxis: { title: hTitle },
		backgroundColor: { fill: 'transparent' },
		series: {
		    0: { targetAxisIndex: 0 },
		    1: { targetAxisIndex: 1 }
		},
		vAxes: {
		    0: { viewWindowMode:'explicit',
			 viewWindow: {
			     max:0,
			     min:30
			 }
		       },
		    1: { viewWindowMode:'explicit',
			 viewWindow: {
			     max:0,
			     min:6
			 },
			 //gridlines: { count: 0 },
		       }
		},
	    };
	    var data = new google.visualization.DataTable();
	    data.addColumn('datetime', 'date');
	    data.addColumn('number', 'temperature');
	    data.addColumn('number', 'vbat');

	    response.forEach(function(e) {
		data.addRow([new Date(e[0]), e[1], e[2]]);
	    });
	    //var data = new google.visualization.arrayToDataTable(response);
	    var cell = document.getElementById(elem + module);
	    //console.log(cell);
	    var chart = new google.visualization.LineChart(cell);
	    chart.draw(data, options);
	},
	error: function(response) {
	    alert("ajax measure failed");
	}
    })
}

function toggleState(module)
{
    var req = {
	'type' : 'toggleState',
	'module' : module
    };
    $.ajax({
	type: 'POST',
	url: 'chart.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    //console.log(response);
	    var cell = document.getElementById('switchBtn' + module);
	    cell.value = response;
	},
	error: function(response) {
	    alert("ajax module failed");
	}
    })
}

function prev()
{
    date.setDate(date.getDate() - 1);
    drawCharts();
}

function next()
{
    date.setDate(date.getDate() + 1);
    drawCharts();
}

$(document).ready(function()
{
    date = new Date();
    width = $("#chart").css("width");

    setElem('status', ID_HEATER_BIG);
    setElem('status', ID_HEATER_SMALL);
    setElem('status', ID_LORA);

    google.charts.setOnLoadCallback(drawCharts);

    $(window).on('resize', function() {
	var w = $("#chart1").css("width");
	if (w != width) {
	    width = w;
	    drawCharts();
	}
    });
})
