#ifndef _ANDROIDMEMDEBUG_H_
#define _ANDROIDMEMDEBUG_H_

#include <iostream>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>


using namespace std;

#include <vector>
#include <pthread.h>


// 支持的搜索类型
enum
{
	DWORD,
	FLOAT,
	BYTE,
	WORD,
	QWORD,
	XOR,
	DOUBLE,
};

// 支持的内存范围(请参考GG修改器内存范围)
enum
{
	Mem_Auto,					// 所以内存页
	Mem_A,
	Mem_Ca,
	Mem_Cd,
	Mem_Cb,
	Mem_Jh,
	Mem_J,
	Mem_S,
	Mem_V,
	Mem_Xa,
	Mem_Xs,
	Mem_As,
	Mem_B,
	Mem_O,
};

struct MemPage
{
	long start;
	long end;
	char flags[8];
	char name[128];
	void *buf = NULL;
};

struct AddressData
{
	long *addrs = NULL;
	int count = 0;
	void freeBuff() {
		free(addrs);
	}
};

typedef struct {
	long offset;
	int value;
}APPEND;

typedef struct {
	vector<APPEND>append;
	void add(long offset,int value) {
		append.push_back({ offset,value });
	}
}OFFSET;

// 根据类型判断类型所占字节大小
size_t judgSize(int type)
{
	switch (type)
	{
	case DWORD:
	case FLOAT:
	case XOR:
		return 4;
	case BYTE:
		return sizeof(char);
	case WORD:
		return sizeof(short);
	case QWORD:
		return sizeof(long);
	case DOUBLE:
		return sizeof(double);
	}
	return 4;
}

int memContrast(char *str,string &flags)
{
	if (strlen(str) == 0 && flags.find("p") != -1)
		return Mem_A;

	if (strstr(str, "/dev/ashmem/") != NULL)
		return Mem_As;

	if (strstr(str, "/system/fonts/") != NULL)
		return Mem_B;

	if (strstr(str, "/data") != NULL && flags.find("r-xp") != -1)
		return Mem_Xa;

	if (strstr(str, "/system") != NULL && flags.find("x") != -1)
		return Mem_Xs;

	if (strcmp(str, "[anon:libc_malloc]") == 0)
		return Mem_Ca;

	if (strstr(str, ":bss") != NULL)
		return Mem_Cb;

	if (strstr(str, "/data/") != NULL && strstr(str, "/lib/") != NULL)
		return Mem_Cd;

	if (strstr(str, "[anon:dalvik") != NULL)
		return Mem_J;

	if (strcmp(str, "[stack]") == 0)
		return Mem_S;

	if (strcmp(str, "/dev/kgsl-3d0") == 0)
		return Mem_V;

	return Mem_O;
}



class MemoryDebug
{
  public:
	pid_t pid = 0;			// 调试应用的PID
	unsigned int threanNum = 0;//进程线程数量

  public:
  //设置调试的应用包名，返回PID
	  int setPackageName(vector<char*>& packName, unsigned char minThreadnum);
    //获取模块的基址，@name：模块名，@index：模块在内存中的内存页位置(第几位，从1开始，默认1)
	  long getModuleBase(const char* name, int index = 1);
	//获取模块的BSS基址
	  long getBssModuleBase(const char* name);
	//读内存的基础函数
	  size_t preadv(long address, void* buffer, size_t size);
	//写内存的基础函数
	size_t pwritev(long address, void *buffer, size_t size);
	
	//根据值搜索内存，并返回相应地址
	template < class T > AddressData search(T value, int type, int mem, bool debug = false);
	//修改内存地址值，返回-1，修改失败，返回1，修改成功
	template < class T > int edit(T value, long address, int type, bool debug = false);
	
	//打印搜索的地址
	void OutAddr(AddressData obj);

	//读取一个DWORD(int)数值
	int ReadDword(long address);
	//读取一个int指针地址数值
	long ReadDword64(long address);
	//读取一个float类型数值
	float ReadFloat(long address);
	//读取一个long类型数值
	long ReadLong(long address);

	unsigned int getProcessThreadNum(pid_t pid);
	AddressData search_DWORD(int value, OFFSET offsets,int mem);
	void setAddrDWORD(long addr, int value);
};

unsigned int MemoryDebug::getProcessThreadNum(pid_t pid) {
	FILE* fpStatus = NULL;
	char statusDir[64] = { 0 };
	unsigned int threadNum = 0;
	char buffer[1024] = { 0 };
	sprintf(statusDir, "/proc/%d/status", pid);
	fpStatus = fopen(statusDir, "r");
	if (fpStatus != NULL) {
		while (fgets(buffer, sizeof(buffer), fpStatus)) {
			if (strstr(buffer, "Threads")) {
				sscanf(buffer, "Threads:%u", &threadNum);
				//puts(buffer);
				//printf("readThread->%u\n", threadNum);
				break;
			}
		}
		fclose(fpStatus);
	}
	return threadNum;
}

int MemoryDebug::setPackageName(vector<char*>&packName,unsigned char minThreadnum)//AndroidMemDebug.cpp
{

	int id = -1;
	DIR *dir;
	FILE *fp;
	char filename[32];
	char cmdline[256];
	struct dirent *entry;
	dir = opendir("/proc");
	if (dir != NULL) {
		while ((entry = readdir(dir)) != NULL)
		{
			id = atoi(entry->d_name);
			if (id != 0)
			{
				sprintf(filename, "/proc/%d/cmdline", id);
				fp = fopen(filename, "r");
				if (fp)
				{
					fgets(cmdline, sizeof(cmdline), fp);
					fclose(fp);
					//printf("%d:%s\n", id, cmdline);
					int i;
					for (i = 0; i < packName.size(); i++) {
						//printf("%s\n", entry);
						//如果结果是null,那么就是没有找到
						if (strstr(cmdline, packName[i]) == NULL)
							break;
					}
					if (i == packName.size() && this->getProcessThreadNum(id) > minThreadnum) {
						closedir(dir);
						return id;
					}
				}
			}
		}
		closedir(dir);
	}
	return -1;
}

long MemoryDebug::getModuleBase(const char* name,int index)
{
	int i = 0;
	long start = 0,end = 0;
    char line[1024] = {0};
    char fname[128];
	sprintf(fname, "/proc/%d/maps", pid);
    FILE *p = fopen(fname, "r");
    if (p)
    {
        while (fgets(line, sizeof(line), p))
        {
            if (strstr(line, name) != NULL)
            {
                i++;
                if(i==index){
                    sscanf(line, "%lx-%lx", &start,&end);
                    break;
                }
            }
        }
        fclose(p);
    }
    return start;
}

long MemoryDebug::getBssModuleBase(const char *name)
{
	FILE *fp;
	int cnt = 0;
	long start;
	char tmp[256];
	fp = NULL;
	char line[1024];
	char fname[128];
	sprintf(fname, "/proc/%d/maps", pid);
	fp = fopen(fname, "r");
	while (!feof(fp))
	{
		fgets(tmp, 256, fp);
		if (cnt == 1)
		{
			if (strstr(tmp, "[anon:.bss]") != NULL)
			{
				sscanf(tmp, "%lx-%*lx", &start);
				break;
			}
			else
			{
				cnt = 0;
			}
		}
		if (strstr(tmp, name) != NULL)
		{
			cnt = 1;
		}
	}
	return start;
}

#ifndef SYS_process_vm_readv
	#define SYS_process_vm_readv 310
#endif // !_SYS_PROCESS_VM_READV

size_t MemoryDebug::preadv(long address, void* buffer, size_t size)
{
	struct iovec iov_ReadBuffer, iov_ReadOffset;
	iov_ReadBuffer.iov_base = buffer;
	iov_ReadBuffer.iov_len = size;
	iov_ReadOffset.iov_base = (void*)address;
	iov_ReadOffset.iov_len = size;
	return syscall(SYS_process_vm_readv, pid, &iov_ReadBuffer, 1, &iov_ReadOffset, 1, 0);
}

#ifndef SYS_process_vm_writev
	#define SYS_process_vm_writev 311
#endif // !SYS_PROCESS_VM_WRITEV

size_t MemoryDebug::pwritev(long address, void *buffer, size_t size)
{
	struct iovec iov_WriteBuffer, iov_WriteOffset;
	iov_WriteBuffer.iov_base = buffer;
	iov_WriteBuffer.iov_len = size;
	iov_WriteOffset.iov_base = (void *)address;
	iov_WriteOffset.iov_len = size;
	return syscall(SYS_process_vm_writev, pid, &iov_WriteBuffer, 1, &iov_WriteOffset, 1, 0);
	//return process_vm_readv(pid, &iov_WriteBuffer, 1, &iov_WriteOffset, 1, 0);
}


template < class T > AddressData MemoryDebug::search(T value, int type, int mem, bool debug)
{
	size_t size = judgSize(type);
	MemPage *mp = NULL;
	AddressData ad;
	long * tmp, *ret = NULL;
	int count = 0;
	char filename[32];
	char line[1024];
	snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);//格式化输出
	FILE *fp = fopen(filename, "r");
	if (fp != NULL)
	{
		//临时存储搜索结果地址，空间大小可以存储1024000条地址，如果觉得不够可以自己加大
		tmp = (long*)calloc(1024000,sizeof(long));
		while (fgets(line, sizeof(line), fp))
		{
			mp = (MemPage*)calloc(1, sizeof(MemPage));
			sscanf(line, "%p-%p %s %*p %*p:%*p %*p   %[^\n]%s", &mp->start, &mp->end, mp->flags, mp->name);


			// 判断内存范围和内存页是否可读(如果内存页不可读，此时如果去
			// 读取这个地址，系统便会抛出异常[信号11,分段错误]并终止调试进程)

			string local_flags;
			local_flags = mp->flags;

			if (strstr(mp->flags, "r") == NULL)
				continue;

			//printf("%p,%p\n", mp->start, mp->end);


			if ((memContrast(mp->name, local_flags) == mem || mem == Mem_Auto))
			{
				//printf("%p-%p\n", mp->start, mp->end);



				mp->buf = (void *)malloc(mp->end - mp->start);//创建一个与内存页同大小的堆空间

				if (mp->buf == NULL)//防止异常
					continue;

				preadv(mp->start, mp->buf, mp->end - mp->start);//读取内存页的大小到自己创建的堆缓冲区

				// 遍历内存页中地址，判断地址值是否和想要的值相同
				for (int i = 0; i < (mp->end - mp->start) / size; i++)
				{
					// 异或类型数值有点特殊，他是游戏防止破解者搜索到
					// 正确数组的一种方式，其加密方式为：值 ^ 地址
					/*
					(
					type == XOR ?
						(*(int *) (mp->buf + i * size) ^ mp->start + i * size)
						:
						*(T *) (mp->buf + i * size)) == value
					)*/
					//如果  (*取泛类型值)(泛类型指针*)(缓冲区起始+偏移字节) == 欲查找的值
					if (*(T*)((long)(mp->buf) + i * size) == value)
					{
						//存储所匹配值的地址到自己创建的缓存地址中
						*(tmp + count) = mp->start + i * size;

						count++;//计数偏移
						if (debug)
						{
							//std::cout<< "index:" << count<< "    value:" << *(T*)((long)mp->buf + i * size)) << std::endl;
								
								/* (type == XOR?*(int *) (mp->buf + i * size) ^ (mp->start + i * size):*(T *) (mp->buf + i * size));*/
							printf("    address:%lX\n", (mp->start) + i * size);
							
						}
					}
				}

				free(mp->buf);
			}
		}
		fclose(fp);
	}
	if(debug)
		printf("搜索结束，共%d条结果\n", count);
	//ret = (long*)calloc(count,sizeof(long));
	//memcpy(ret,tmp,count*(sizeof(long)));//把tmp数据内存拷贝到ret里面
	//free(tmp);//释放tmp
	//ad.addrs = ret;//把ret的首部指针赋值给地址结构数据成员中,指向地址数据成员

	ad.addrs = (long*)calloc(count, sizeof(unsigned long));
	memcpy(ad.addrs, tmp, count * (sizeof(long)));

	ad.count = count;//赋值搜索的地址数量
	free(ret);
	return ad;
}

template < class T > int MemoryDebug::edit(T value,long address,int type,bool debug)
{
	if(-1 == pwritev(address,&value,judgSize(type)))
	{
		if(debug)
		printf("修改失败-> addr:%lX\n",address);
		return -1;
	}else
	{
		if(debug)
		printf("修改成功-> addr:%lX\n",address);
		return 1;
	}
	return -1;
}

AddressData MemoryDebug::search_DWORD(int value, OFFSET offsets,int men) {
	AddressData temp = this->search<int>(value, DWORD, men, false);
	AddressData ret;
	vector<long>findAddr;
	int i, j;
	//this->OutAddr(temp);
	for (i = 0; i < temp.count; i++){
		for (j = 0; j < offsets.append.size(); j++){
			if (this->ReadDword(temp.addrs[i] + offsets.append[j].offset) != offsets.append[j].value)
				break;
		}
		if (j == offsets.append.size())
		{
			findAddr.push_back(temp.addrs[i]);
		}
	}
	memset(&ret, 0, sizeof(ret));
	ret.addrs = (long*)malloc(sizeof(long) * findAddr.size());//分配足够的堆空间
	ret.count = findAddr.size();
	for (i = 0; i < findAddr.size(); i++)
		ret.addrs[i] = findAddr[i];
	findAddr.clear();
	return ret;
}

void MemoryDebug::setAddrDWORD(long addr,int value) {
	this->edit<int>(value, addr, DWORD, false);
}

void MemoryDebug::OutAddr(AddressData obj) {
	printf("地址数量:%d\n", obj.count);
	for (int i = 0; i < obj.count; i++) {
		printf("%lX\n", obj.addrs[i]);
	}
}

long MemoryDebug::ReadDword64(long address)
{
	long local_ptr = 0;
	preadv(address, &local_ptr, 4);
	return local_ptr;
}

int MemoryDebug::ReadDword(long address)
{
	int local_value = 0;
	preadv(address, &local_value, 4);
	return local_value;
}

float MemoryDebug::ReadFloat(long address)
{
	float local_value = 0;
	preadv(address, &local_value, 4);
	return local_value;
}

long MemoryDebug::ReadLong(long address)
{
	long local_value = 0;
	preadv(address, &local_value, 8);
	return local_value;
}

void getRoot(char **argv)
{
	char shellml[64];
	sprintf(shellml, "su -c %s", *argv);
	if (getuid() != 0)
	{
		system(shellml);
		exit(1);
	}
}


#endif
