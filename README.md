# QtResArscEditor
android arsc file editor by Qt  
安卓的arsc资源文件编辑器，网上的JAVA写的好像不那么好用，所以自己写一个  
当然写的过程中也参考很多他们的代码  
使用方法：  
1.在你的winrar的“设置->集成”页，把apk加到自定义扩展名里面，这会方便以后的文件处理，不要用winzip或者7zip修改apk文件，不要问我原因，反正我用它们修改文件后，安装可以，但无法执行  
2.打开apk文件，把里面的resources.arsc解压出来  
3.用QtResArscEditor打开resources.arsc  
4.查看要翻译的资源，比如array,string等，看是否有我们需要的本地化语言  
5.如果没有，在树状结构上右键，选AddLocale，增加一个语言，程序会把所有可翻译的内容使用默认语言数据填充  
6.如果已经有这个语言，但是里面的翻译内容不全，可在右侧点右键，选AddValue，添加一条内容，或者选AddAllValue添加所有可翻译内容  
7.对每条内容都可以手动选中翻译  
8.也可在树状结构上选中ExportLocale，把内容导出成XML文件进行修改翻译，然后选中ImportLocale导入，完成后保存resources.arsc  
9.用winrar将修改后的resources.arsc重新写回apk文件，可选中不压缩  
10.使用签名工具签名后就可以安装了  