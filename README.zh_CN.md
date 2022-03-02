# ChromiumUI
ue4 webbrowser plugin with cef3 version chromium-84.0.4147.38

webbrowser插件版本

ue4.27 提供的cef3 版本过低，无法支持es6语法的html网页.
升级引擎的cef3需要在源码版本上处理。由于源码版本的datasmith版权原因无法提供关于cad相关的导入。
所以制作了webbrowser的插件版本
独立提供cef3 chromium-84.0.4147.38版本内核。可以在非源码版本的ue4.27中使用

因为github的大文件限制，使用前需要解压缩Source\ThirdParty\ChromiumUILibrary文件夹下面的ChromiumUILibrary.7z
解压缩在当前目录，就可以完整使用插件

js和ue4相互调用使用说明，该例子在插件Content目录

1.绑定桥接对象  
![Alt text](https://github.com/shiniu0606/ChromiumUI/blob/main/doc/1.PNG?raw=true "Optional Title")  

2.定义蓝图函数(注意该函数在js中为小写)  
![Alt text](https://github.com/shiniu0606/ChromiumUI/blob/main/doc/2.PNG?raw=true "Optional Title")  

3.js调用ue4l蓝图函数  
![Alt text](https://github.com/shiniu0606/ChromiumUI/blob/main/doc/3.PNG?raw=true "Optional Title")  