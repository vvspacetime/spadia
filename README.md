# WebRTC PeerConnection implement

### data flow
```
PeerConnection                                         MediaStream               
RecvFromSocket()=>rstream.OnRTP() -------------------> OnRTP()=>sink.OnMediaData()


MediaStream                                      PeerConnection
OnRTP()=>sink.OnMediaData()  ---------------->   OnMediaData()=>SendToSocket    
``` 


### dependents
libuv, glog