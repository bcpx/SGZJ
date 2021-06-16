#include "AndroidMemDebug.h"

MemoryDebug mem;


typedef struct {
	long address;
	int Type;
	int value;
	void freevalue() {
		//mem.preadv(address, (int*)&value, 4);
		mem.edit<int>(value, address, DWORD, false);
	}

}*PFREE_ADDR, FREE_ADDR;

vector<FREE_ADDR>g_freelist;//冻结地址列表
pthread_t g_freeThread;//东冻线程句柄
bool g_freeControl = false;//冻结控制结束
uint32_t g_freeSleep = 100000;//冻结地址间隔1000000=1s

//冻结函数
void* pthread_freelistFun(void* arg) {
	while (g_freeControl) {
		for (int i = 0; i < g_freelist.size(); i++) {
			g_freelist[i].freevalue();
		}
		usleep(g_freeSleep);//冻结间隔
	}
	return nullptr;
}

//地址添加到列表
void freeAddList(FREE_ADDR obj) {
	g_freelist.push_back(obj);
}

//冻结开始
void freeStart() {
	g_freeControl = true;
	pthread_create(&g_freeThread, nullptr, pthread_freelistFun, nullptr);
}

//设置冻结间隔,1000000=1s
void freeSleep(uint32_t value) {
	g_freeSleep = value;
}

//冻结束停止
void freeStop() {
	g_freeControl = false;
}

int main(int argc, char** argv) {

	printf("uid:%u\n", geteuid());

	vector<char*>packHash;
	//packHash.push_back((char*)"com.enjoymi.sgzj.mi\0");//你的
	//packHash.push_back((char*)"com.tencent.tmgp.enjoymisgzj");//我的

	packHash.push_back((char*)"sgzj");
	packHash.push_back((char*)"enjoy");

	mem.pid = mem.setPackageName(packHash);
	mem.threanNum = mem.getProcessThreadNum(mem.pid);

	printf("Find game pid = %d\n", mem.pid);
	printf("Game threadNum = %u\n", mem.threanNum);

	OFFSET off1;
	off1.add(-0x70, 30108);
	off1.add(0x70, 30106);
	AddressData first = mem.search_DWORD(30107, off1, Mem_A);

	
	long zhanglingjian;
	if (first.count > 0)//判断返回的地址结果数量是否>0
		zhanglingjian = mem.ReadDword(first.addrs[0] - 0x14);

	printf("zhanglingjian:0x%X|%ld\n", zhanglingjian, zhanglingjian);

	OFFSET off2;
	off2.add(-0x70, 44);
	off2.add(-0x48, 200);
	off2.add(-0x38, 1);
	AddressData second = mem.search_DWORD(40, off2, Mem_A);
	
	FREE_ADDR freeAddr;
	if (second.count > 0)//判断返回的地址结果数量是否>0
	{
		freeAddr.address = second.addrs[0] - 0x14;
		freeAddr.value = zhanglingjian;
	}
	
	freeAddList(freeAddr);//添加这个地址到冻结列表

	printf("free list cat:\n");
	for (FREE_ADDR obj : g_freelist) 
	{
		printf("address:0x%lX|value:0x%X %d\n", obj.address, obj.value, obj.value);//输出一下冻结列表,查看一下内容是否正确,可有可无
	}

	printf("set free ms is:0.1s\n");//1000000=1s
	freeSleep(100000);
	printf("start free\n");
	freeStart();


	uint32_t cnt = 0;

	while (cnt < 1) {
		printf("%d\n", cnt);
		usleep(1000000);//阻塞 主线程查看冻结效果
		cnt++;
	}
	


	first.freeBuff();//释放搜索的地址内存数据
	second.freeBuff();

	off1.append.clear();//释放掉添加的偏移数据
	off2.append.clear();

	g_freelist.clear();//释放冻结列表数据
	
	//testxg

	return EXIT_SUCCESS;
}