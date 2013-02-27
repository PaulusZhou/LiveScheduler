#!/bin/bash 
Src1=rtsp://222.201.146.125/H264
Src2=rtsp://222.201.146.125/H264
Src3=rtsp://222.201.146.125/H264
Src4=rtsp://222.201.146.125/H264
#Src5=rtsp://222.201.146.81/H264
#Src6=rtsp://222.201.146.81/H264
#Src7=rtsp://222.201.146.81/H264
#Src8=rtsp://222.201.146.81/H264
nohup ./live555ProxyServer $Src1 $Src2 $Src3 $Src4 & #$Src5 $Src6 $Src7 $Src8 & 
