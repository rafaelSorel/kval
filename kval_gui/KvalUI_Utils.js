

String.prototype.isEmpty = function(s) {
    return (s === '');
};


function hourFromTime(time) {
    var hour = time.substring(0, time.indexOf(':'));
    return hour;
}

function minuteFromTime(time) {
    var minute = time.substring(time.indexOf(':'), 5);
    return minute;
}

function scaled(x) {
    return x * scaleRatio;
}
