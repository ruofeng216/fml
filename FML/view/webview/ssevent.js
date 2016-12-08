(function(global) {
  window.ssevent = null;
  var callerObj = {
    command: 1
  }; // 临时存储js调用者对象，key:command，value:对象

  new QWebChannel(qt.webChannelTransport, function(channel) {
    window.ssevent = channel.objects.ssevent;
    ssevent.sigCppResult.connect(function(caller, params) {
      console.log('caller:' + caller + ',params:' + params.toString());
      if (callerObj[caller] != 'undefined') {
        callerObj[caller].response(params);
        delete callerObj[caller];
      } else {
        console.warn(caller + ' is undefined');
      }
    });
    ssevent.init();
  });

  global.getCommand = function() {
    callerObj.command = (callerObj.command + 1) % 1000000;
    return callerObj.command;
  };

  // 调用规则：runcpp(function(command) {}, function(jsonData) {});
  // 调用C++接口，异步处理返回值
  global.runcpp = function(requestFunc, responseFunc) {
    var func = {};
    func.request = requestFunc;
    func.response = responseFunc;
    var cmd = getCommand();
    callerObj[cmd] = func;
    requestFunc(cmd);
  };

  // 调用slotHandle C++公共接口，无返回值
  global.runcpp_no_return = function(val) {
    ssevent.slotHandle(getCommand(), val);
  };

  // 调用slotHandle C++公共接口，异步处理C++的返回值
  global.runcpp_return = function(val, responseFunc) {
    global.runcpp(function(command) {
      ssevent.slotHandle(command, val);
    }, responseFunc);
  };
})(this);
