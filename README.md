## Remote File System (RFS)

### **1. Overview**

RFS is a client-server application that allows clients to perform file operations on a remote server over TCP sockets. Clients can upload, download, and delete files on the server.

### **2. Features**

​	•	**WRITE**: Upload a local file to the server.

​	•	**GET**: Download a file from the server.

​	•	**RM**: Delete a file or directory on the server.

### **3. Compilation**

Use the provided Makefile to compile:

```bash
make all
```

This generates:

​	•	server: The server executable.

​	•	rfs: The client executable.

### **4.Usage**

#### **1.Start the Server**

```bash
./rfserver
```

​	•	Listens on the specified port.

​	•	Creates server_root/ if it doesn’t exist.



#### **2. Client Commands**

##### **a. WRITE**

Upload a local file to the server.

```bash
./rfs WRITE <local_file> <remote_file>
```

**Example:**

```bash
./rfs WRITE test_files/test_upload.txt remote/uploaded_file.txt     
```

##### **b. GET**

Download a file from the server.

```bash
./rfs GET <remote_file> <local_file>
```

**Example:**

```bash
./rfs GET remote/uploaded_file.txt local/test2.txt 
```

##### **c. RM**

Delete a file or directory on the server.

```
./rfs RM <remote_path>
```

**Example:**

```
./rfs RM remote/uploaded_file.txt
```



### **5. Notes**

​	•	Server paths are relative to server_root/.

​	•	Directories are created as needed during uploads.

​	•	Ensure correct remote paths in commands.

**Limitations**

​	•	Single-client handling (no concurrency).

​	•	No authentication or encryption.

​	



