http-server -p 3000 &
chrome --disable-webrtc-encryption --enable-logging --vmodule=*/webrtc/*=9 http://127.0.0.1:3000