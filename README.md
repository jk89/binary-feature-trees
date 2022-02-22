# Binary-feature-trees v0.0.1

Based of the paper "Bags of Binary Words for Fast Place Recognition in
Image Sequences, Dorian G ́alvez-L  ́opez and Juan D. Tard  ́os, Member, IEEE" found here:
http://doriangalvez.com/papers/GalvezTRO12.pdf

# Installation notes

1. install catkin-tools

sudo apt get install catkin
sudo pip3 install git+https://github.com/catkin/catkin_tools.git
sudo apt-get install python3-catkin-pkg

2. init catkin workspace

catkin_make


3. catkin config

catkin config --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

4. clean

catkin clean -y

5. build

catkin build

ctrl+p
ext install ms-vscode.cpptools
ext install betwo.b2-catkin-tools


https://marketplace.visualstudio.com/items?itemName=betwo.b2-catkin-tools


6. catkin build
./devel/lib/trainer/trainer 
Hello World2


# Sanity checker JS script

var results = [];
Object.keys(a.clusterMembers).forEach((idx) => {
console.log(idx);
console.log("x", a.clusterMembers[idx][1].length);
    Object.keys(a.clusterMembers[idx][1]).forEach((m, memberIdx) => {
    	if (a.clusterMembers[idx][1][memberIdx] == 490) {
    		results.push([idx, memberIdx]);
    	}
    });
});