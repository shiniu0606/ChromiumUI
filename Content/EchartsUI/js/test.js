window.onload=function(){

    "object"==typeof ue&&"object"==typeof ue.interface

    document.getElementById("button1").addEventListener("click",function(){
            ue4("button",{"data":"按钮一按下了"});
        });
    document.getElementById("button2").addEventListener("click",function(){
            ue4("button",{"data":"按钮二按下了"});
        });
    document.getElementById("button3").addEventListener("click",function(){
            ue4("button",{"data":"按钮三按下了"});
        });
    document.getElementById("button4").addEventListener("click",function(){
            ue4("button",{"data":"按钮四按下了"});
        });
    document.getElementById("button5").addEventListener("click",function(){
            ue4("button",{"data":"按钮五按下了"});
        });
        ue.interface.SetTitle = function(title){
            document.getElementById("title").innerHTML = title
        }
};