## Remote File System (RFS)

* **Name**: Zhengpeng Qiu and Blake Koontz

* **Course**: CS5600

* **Semester**: Fall 2024

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
#### **3. Multiple Clients**

To handle multiple clients you can create multiple clients connected to the server

For testing purposes:
Use SSH to create a session and begin running the server.
```
./rfserver
```

Next create a new SSH session to simulate client 1.
Now begin to enter commands or run a bash script for commands.
For example:
```bash
./rfs WRITE <local_file> <remote_file>
```
Next create a new SSH session to simulate client 2.
Now begin to enter commands or run a bash script for commands.
For example:
```bash
./rfs WRITE <local_file> <remote_file>
```

#### **4. Stop the server**

Press **CTRL+C** in the server terminal, and you will see:

```bash
Caught SIGINT!
```

Reference: When you stop a process with CTRL-C, it'll exit by default leaving ports open and potentially data unset. So, it is best to "catch" or "trap" the SIGINT signal and add your own behavior so you can do a "safe" exit... [https://www.delftstack.com/howto/c/sigint-in-c/Links to an external site.](https://www.delftstack.com/howto/c/sigint-in-c/)	

### **5. Notes**

​	•	Server paths are relative to server_root/.

​	•	Directories are created as needed during uploads.

​	•	Ensure correct remote paths in commands.

**Limitations**

​	•	No authentication or encryption.
