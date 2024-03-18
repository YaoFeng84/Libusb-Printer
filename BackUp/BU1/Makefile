# 定义编译器和链接器 
CXX = g++  
  
# 定义编译选项  (显示所有警告，标准采用C++11格式)
CXXFLAGS = -Wall -g -std=c++11  
  
# 定义头文件搜索路径  
INCLUDE_DIRS = -I/usr/local/include/libusb-1.0  
  
# 库文件目录和库名  
LIB_DIRS = -L/usr/local/lib  
LIBS = -lusb-1.0 -lpthread  
  
# 源文件列表  (支持不同路径的源文件)
SRCS = ./PrintUSB3.cpp \  
       ./FUN_PrinterUSB.cpp  

# 自动生成目标文件列表
OBJS = $(SRCS:.cpp=.o)  
  
# 默认生成目标文件名为 PrintUSB3  
all: PrintUSB3  
  
# 链接目标文件生成可执行文件  
PrintUSB3: $(OBJS)  
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) -o $@ $^ $(LIBS)  
  
# 编译规则，为每个.cpp文件生成对应的.o文件  
%.o: %.cpp  
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@  
  
# 清理生成的文件  
clean:  
	rm -f $(OBJS) PrintUSB3  
  
# 如果需要，可以添加伪目标来执行特定的任务，比如检查语法  
.PHONY: check  
check:  
	$(CXX) $(CXXFLAGS) -fsyntax-only $(SRCS)
