# Binary-feature-trees v0.0.1



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


6.

catkin build
./devel/lib/trainer/trainer 
Hello World2