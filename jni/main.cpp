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

vector<FREE_ADDR>g_freelist;//�����ַ�б�
pthread_t g_freeThread;//�����߳̾��
bool g_freeControl = false;//������ƽ���
uint32_t g_freeSleep = 100000;//�����ַ���1000000=1s

//���ắ��
void* pthread_freelistFun(void* arg) {
	while (g_freeControl) {
		for (int i = 0; i < g_freelist.size(); i++) {
			g_freelist[i].freevalue();
		}
		usleep(g_freeSleep);//������
	}
	return nullptr;
}

//��ַ��ӵ��б�
void freeAddList(FREE_ADDR obj) {
	g_freelist.push_back(obj);
}

//���Ὺʼ
void freeStart() {
	g_freeControl = true;
	pthread_create(&g_freeThread, nullptr, pthread_freelistFun, nullptr);
}

//���ö�����,1000000=1s
void freeSleep(uint32_t value) {
	g_freeSleep = value;
}

//������ֹͣ
void freeStop() {
	g_freeControl = false;
}

int main(int argc, char** argv) {

	printf("uid:%u\n", geteuid());

	vector<char*>packHash;
	//packHash.push_back((char*)"com.enjoymi.sgzj.mi\0");//���
	//packHash.push_back((char*)"com.tencent.tmgp.enjoymisgzj");//�ҵ�

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
	if (first.count > 0)//�жϷ��صĵ�ַ��������Ƿ�>0
		zhanglingjian = mem.ReadDword(first.addrs[0] - 0x14);

	printf("zhanglingjian:0x%X|%ld\n", zhanglingjian, zhanglingjian);

	OFFSET off2;
	off2.add(-0x70, 44);
	off2.add(-0x48, 200);
	off2.add(-0x38, 1);
	AddressData second = mem.search_DWORD(40, off2, Mem_A);
	
	FREE_ADDR freeAddr;
	if (second.count > 0)//�жϷ��صĵ�ַ��������Ƿ�>0
	{
		freeAddr.address = second.addrs[0] - 0x14;
		freeAddr.value = zhanglingjian;
	}
	
	freeAddList(freeAddr);//��������ַ�������б�

	printf("free list cat:\n");
	for (FREE_ADDR obj : g_freelist) 
	{
		printf("address:0x%lX|value:0x%X %d\n", obj.address, obj.value, obj.value);//���һ�¶����б�,�鿴һ�������Ƿ���ȷ,���п���
	}

	printf("set free ms is:0.1s\n");//1000000=1s
	freeSleep(100000);
	printf("start free\n");
	freeStart();


	uint32_t cnt = 0;

	while (cnt < 1) {
		printf("%d\n", cnt);
		usleep(1000000);//���� ���̲߳鿴����Ч��
		cnt++;
	}
	


	first.freeBuff();//�ͷ������ĵ�ַ�ڴ�����
	second.freeBuff();

	off1.append.clear();//�ͷŵ���ӵ�ƫ������
	off2.append.clear();

	g_freelist.clear();//�ͷŶ����б�����
	
	//testxg

	return EXIT_SUCCESS;
}