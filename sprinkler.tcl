# This script is created by NSG2 beta1
# <http://wushoupong.googlepages.com/nsg>

#===================================
#     Simulation parameters setup
#===================================
Antenna/OmniAntenna set Gt_ 1              ;#Transmit antenna gain
Antenna/OmniAntenna set Gr_ 1              ;#Receive antenna gain
set val(chan)   Channel/WirelessChannel    ;# channel type
set val(prop)   Propagation/TwoRayGround   ;# radio-propagation model
set val(netif)  Phy/WirelessPhy            ;# network interface type
set val(mac)    Mac/802_11                 ;# MAC type
set val(ifq)    Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)     LL                         ;# link layer type
set val(ant)    Antenna/OmniAntenna        ;# antenna model
set val(ifqlen) 50                         ;# max packet in ifq
set val(rp)     AODV                       ;# routing protocol
set val(x)      5                          ;# X dimension of topography
set val(y)      5                          ;# Y dimension of topography
set val(nn)     [expr $val(x)*$val(y)]     ;# number of mobilenodes
set val(start)  1.0                        ;# time of simulation end
set val(stop)   200.0                      ;# time of simulation end
set val(pktNum) 60
set val(head)   5

#===================================
#        Initialization        
#===================================
#Create a ns simulator
set ns [new Simulator]

#Setup topography object
set topo       [new Topography]
$topo load_flatgrid $val(x) $val(y)
create-god $val(nn)

#Open the NS trace file
set tracefile [open out.tr w]
$ns trace-all $tracefile

#Open the NAM trace file
set namfile [open out.nam w]
$ns namtrace-all $namfile
$ns namtrace-all-wireless $namfile $val(x) $val(y)
set chan [new $val(chan)];#Create wireless channel

#===================================
#     Mobile node parameter setup
#===================================
$ns node-config -adhocRouting  $val(rp) \
                -llType        $val(ll) \
                -macType       $val(mac) \
                -ifqType       $val(ifq) \
                -ifqLen        $val(ifqlen) \
                -antType       $val(ant) \
                -propType      $val(prop) \
                -phyType       $val(netif) \
                -channel       $chan \
                -topoInstance  $topo \
                -agentTrace    ON \
                -routerTrace   ON \
                -macTrace      ON \
                -movementTrace ON
                #-energyModel "EnergyModel" \
                #-initialEnergy 1 \
                #-rxPower 0.3 \
                #-txPower 0.3 \     
                #           
# For model 'TwoRayGround'
# 2径模型,包括直射波和地面反射波
set dist(5m)  7.69113e-06
set dist(9m)  2.37381e-06
set dist(10m) 1.92278e-06
set dist(11m) 1.58908e-06
set dist(12m) 1.33527e-06
set dist(13m) 1.13774e-06
set dist(14m) 9.81011e-07
set dist(15m) 8.54570e-07
set dist(16m) 7.51087e-07
set dist(20m) 4.80696e-07
set dist(25m) 3.07645e-07
set dist(30m) 2.13643e-07
set dist(35m) 1.56962e-07
set dist(40m) 1.20174e-07
Phy/WirelessPhy set CSThresh_ $dist(10m) ;#功率范围
Phy/WirelessPhy set RXThresh_ $dist(10m)

#===================================
#        Nodes Definition        
#===================================
# Create 2 nodes
# set n0 [$ns node]
# $n0 set X_ 300
# $n0 set Y_ 300
# $n0 set Z_ 0.0
# $ns initial_node_pos $n0 20
# set n1 [$ns node]
# $n1 set X_ 545
# $n1 set Y_ 300
# $n1 set Z_ 0.0
# $ns initial_node_pos $n1 20

# Create nodes
for {set i 0} {$i < $val(y) } {incr i} {
 for {set j 0} {$j < $val(x) } {incr j} {
  set node_([expr $i*$val(x)+$j]) [$ns node]
  # puts "Create node_([expr $i*$val(x)+$j])" ;
  $node_([expr $i*$val(x)+$j]) set X_ [expr $j * 8]
  $node_([expr $i*$val(x)+$j]) set Y_ [expr $i * 8]
  $node_([expr $i*$val(x)+$j]) set Z_ 0.0
  $node_([expr $i*$val(x)+$j]) random-motion 0 ; # disable random motion
  $ns initial_node_pos $node_([expr $i*$val(x)+$j]) 4 ; #  defines the node size in nam
 }
}

# Add neighbors (ID of nodes)
# for {set i 0} {$i < $val(y) } {incr i} {
#  for {set j 0} {$j < $val(x) } {incr j} {
#   set tips "";

#   if { $i == 0 } {
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr ($i+1)*$val(y)+$j]) id]; #下邻居
#     append tips " " "node_([expr ($i+1)*$val(y)+$j])";
#   } elseif { $i > 0 && $i < $val(y)-1} {
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr ($i-1)*$val(y)+$j]) id]; #上邻居
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr ($i+1)*$val(y)+$j]) id]; #下邻居
#     append tips " " "node_([expr ($i-1)*$val(y)+$j])";
#     append tips " " "node_([expr ($i+1)*$val(y)+$j])";
#   } else {
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr ($i-1)*$val(y)+$j]) id]; #上邻居
#     append tips " " "node_([expr ($i-1)*$val(y)+$j])";
#   }

#   if { $j == 0 } {
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr $i*$val(x)+$j+1]) id]; #右邻居
#     append tips " " "node_([expr $i*$val(x)+$j+1])";
#   } elseif { $j > 0 && $j < $val(x)-1} {
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr $i*$val(x)+$j-1]) id]; #左邻居
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr $i*$val(x)+$j+1]) id]; #右邻居
#     append tips " " "node_([expr $i*$val(x)+$j-1])";
#     append tips " " "node_([expr $i*$val(x)+$j+1])";
#   } else {
#     $node_([expr $i*$val(x)+$j]) add-neighbor [$node_([expr $i*$val(x)+$j-1]) id]; #左邻居
#     append tips " " "node_([expr $i*$val(x)+$j-1])";
#   }
#   puts "node_([expr $i*$val(x)+$j]): [$node_([expr $i*$val(x)+$j]) neighbors]" ;
#  }
# }

#===================================
#        Agents Definition        
#===================================
# set sT0 [new Agent/Sprinkler]
# $ns attach-agent $n0 $sT0
# $sT0 set nodeID 0

# set sT1 [new Agent/Sprinkler]
# $ns attach-agent $n1 $sT1
# $sT1 set nodeID 1

# $ns connect $sT0 $sT1

# attach-agent方法将代理对象和节点进行绑定
for {set i 0} {$i < $val(nn) } {incr i} {
  set spl_($i) [new Agent/Sprinkler]
  $ns attach-agent $node_($i) $spl_($i)
}


# connect方法建立代理对象的端到端连接
for {set i 0} {$i < [expr $val(nn) - 1] } {incr i} {
  for {set j [expr $i + 1]} {$j < $val(nn) } {incr j} {
    $ns connect $spl_($i) $spl_($j)
  }
}


#===================================
#        Applications Definition        
#===================================

# 计算开始节点
if {$val(x) % 3 == 1} {
  set $val(head) 0;
} else {
  set $val(head) $val(y);
}

# 统一设置变量
for {set i 0} {$i < $val(y) } {incr i} {
  for {set j 0} {$j < $val(x) } {incr j} {
    $spl_([expr $i*$val(x)+$j]) set nodeID [expr $i*$val(x)+$j]; #nodeID
    $spl_([expr $i*$val(x)+$j]) set maxRow $val(y); #maxRow
    $spl_([expr $i*$val(x)+$j]) set maxCol $val(x); #maxCol
    $spl_([expr $i*$val(x)+$j]) set row $i; #maxCol
    $spl_([expr $i*$val(x)+$j]) set col $j; #maxCol
    $spl_([expr $i*$val(x)+$j]) set pktNum $val(pktNum); #maxCol
    $spl_([expr $i*$val(x)+$j]) initSprinkler; #初始化其它变量
  }
}

#不同数据包的颜色

#===================================
#        Termination        
#===================================
#Define a 'finish' procedure
proc finish {} {
    global ns tracefile namfile
    $ns flush-trace
    close $tracefile
    close $namfile
    # exec nam out.nam &
    exit 0
}

# $spl_(0) startSprinkler; #启动startSprinkler函数

$ns at $val(start) "$spl_($val(head)) startSprinkler"

for {set i 0} {$i < $val(nn) } { incr i } {
    $ns at $val(stop) "$node_($i) reset"
}

$ns at $val(stop) "finish"
# $ns at $val(stop) "$ns nam-end-wireless $val(stop)"
$ns run
