http-server -p 3000 &
chrome --disable-webrtc-encryption --use-fake-device-for-media-stream \
       --use-fake-ui-for-media-stream --allow-hidden-media-playback \
       --enable-logging --vmodule=*/webrtc/*=9 http://127.0.0.1:3000