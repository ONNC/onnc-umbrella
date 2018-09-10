CXXFLAGS = -std=c++14 -DONNX_NAMESPACE=onnx
INCLUDE_DIRS = $(INSTALL_DIR)/include $(EXTERNAL_DIR)/include
LIBRARY_DIRS = $(INSTALL_DIR)/lib $(EXTERNAL_DIR)/lib
LFLAGS = -lonnc -lonnx -lonnx_proto -lprotobuf

EXEC = onnx-dis

.PHONY: all clean install

all: $(BUILD_DIR)/$(EXEC)

$(BUILD_DIR)/$(EXEC) : main.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(EXEC) $(addprefix -I, $(INCLUDE_DIRS)) $(addprefix -L, $(LIBRARY_DIRS)) $< $(LFLAGS)

install: $(BUILD_DIR)/$(EXEC)
	cp $(BUILD_DIR)/$(EXEC) $(INSTALL_DIR)/bin/$(EXEC)

clean:
	rm -f $(BUILD_DIR)/$(EXEC)