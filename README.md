# ChromiumUI

- zh_CN [简体中文](README.zh_CN.md)

ue4 webbrowser plugin with cef3 version chromium-84.0.4147.38

# Precautions
need check disable WebBrowser,SteamVR,OnlineFramework plugin

#How to usage

cd Source\ThirdParty\ChromiumUILibrary

unzip ChromiumUILibrary.7z in this folder

copy plugin to project use


#using js and ue4 to call each other, the example is in the plugin Content directory

1.bind bridge object  
![Alt text](https://github.com/shiniu0606/ChromiumUI/blob/main/doc/1.PNG?raw=true "Optional Title")  

2.define the blueprint function (note that the function is lowercase in js)   
![Alt text](https://github.com/shiniu0606/ChromiumUI/blob/main/doc/2.PNG?raw=true "Optional Title")  

3.call ue4 function  
![Alt text](https://github.com/shiniu0606/ChromiumUI/blob/main/doc/3.PNG?raw=true "Optional Title")    
