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
	//com.tencent.tmgp.gqdfzz
	packHash.push_back((char*)"gqdfzz");

	mem.pid = mem.setPackageName(packHash, 100);
	mem.threanNum = mem.getProcessThreadNum(mem.pid);


	printf("Find game pid = %d\n", mem.pid);
	printf("Game threadNum = %u\n", mem.threanNum);

	OFFSET xs;
	
	AddressData adrdata = mem.search_DWORD(1065353216, xs, Mem_Cd);
	mem.OutAddr(adrdata);



	return EXIT_SUCCESS;
}