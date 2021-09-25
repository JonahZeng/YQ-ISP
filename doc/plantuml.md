# plantuml
活动图绘制指南：
[地址](https://plantuml.com/zh/activity-diagram-legacy)

示例：

```uml
@startuml
(*) -right-> "fileread" 
"fileread" -right-> "fe_firmware" 
"fe_firmware" -right-> [14bit]"blc" 
"fe_firmware" -right-> [reg]"blc" 
"blc" -right-> [14bit]"lsc"
"fe_firmware" -right-> [reg]"lsc"
"lsc" -right-> [14bit]"awb gain"
"fe_firmware" -right-> [reg]"awb gain"
"awb gain" -right-> [14bit]"demosaic"
"fe_firmware" -right-> [reg]"demosaic"
"demosaic" -right-> (*)
@enduml
```