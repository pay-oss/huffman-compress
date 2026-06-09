#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SYM 256
#define MAGIC 0x48434646  // 文件魔数，检验文件合法性

typedef struct HNode {
	unsigned char sym;
	int freq;
	struct HNode *left, *right;
} HNode;

typedef struct {
	HNode *node;
	int freq;
} PQItem;

HNode* node_create(unsigned char s, int f) {
	HNode *n = (HNode*)malloc(sizeof(HNode));
	n->sym = s;
	n->freq = f;
	n->left = n->right = NULL;
	return n;
}

HNode* build_tree(int freq[]) {//建树
	PQItem pq[MAX_SYM];
	int n = 0;
	
	for (int i = 0; i < MAX_SYM; i++){
		if (freq[i] > 0){
			HNode*newNode=node_create(i,freq[i]);
			pq[n].node=newNode;
			pq[n].freq=freq[i];
			n++;
			
		}
	}
	
	if (n == 0) return NULL;
	if (n == 1) return node_create(0, freq[0]);
	
	for (int i = 0; i < n - 1; i++) {
		for (int j = i + 1; j < n; j++) {
			if (pq[i].freq > pq[j].freq) {
				PQItem t = pq[i];
				pq[i] = pq[j];
				pq[j] = t;
			}
		}
	}
	
	while (n > 1) {
		HNode *p = node_create(0, pq[0].freq + pq[1].freq);
		p->left = pq[0].node;
		p->right = pq[1].node;
		
		pq[0].node = p;
		pq[0].freq = p->freq;
		for (int i = 1; i < n - 1; i++)
			pq[i] = pq[i + 1];
		n--;
		
		for (int i = 0; i < n - 1; i++) {
			for (int j = i + 1; j < n; j++) {
				if (pq[i].freq > pq[j].freq) {
					PQItem t = pq[i];
					pq[i] = pq[j];
					pq[j] = t;
				}
			}
		}
	}
	return pq[0].node;
}

void gen_code(HNode *root, char *buf, int d, char *code[], int len[]) {
	if (!root) return;
	if (!root->left && !root->right) {
		code[root->sym] = (char*)malloc(d + 1);
		strncpy(code[root->sym], buf, d);
		code[root->sym][d] = '\0';
		len[root->sym] = d;
		return;
	}
	if (root->left) {
		buf[d] = '0';
		gen_code(root->left, buf, d + 1, code, len);
	}
	if (root->right) {
		buf[d] = '1';
		gen_code(root->right, buf, d + 1, code, len);
	}
}

int compress_file(const char *in, const char *out, long *orig, long *comp) {
	FILE *fi = fopen(in, "rb");
	if (!fi) {
		perror("打开输入文件失败");
		return -1;
	}
	FILE *fo = fopen(out, "wb");
	if (!fo) {
		perror("打开输出文件失败");
		fclose(fi);
		return -1;
	}
	
	int freq[MAX_SYM] = {0};
	unsigned char c;
	
	fseek(fi, 0, SEEK_END);
	*orig = ftell(fi);
	rewind(fi);
	
	while (fread(&c, 1, 1, fi) == 1) freq[c]++;
	
	HNode *root = build_tree(freq);
	char *code[MAX_SYM] = {NULL};
	int len[MAX_SYM] = {0};
	char tmp[256];
	
	gen_code(root, tmp, 0, code, len);
	
	int magic = MAGIC;
	fwrite(&magic, sizeof(int), 1, fo);
	fwrite(freq, sizeof(int), MAX_SYM, fo);
	
	unsigned char buf = 0;
	int bit = 0;
	
	rewind(fi);
	while (fread(&c, 1, 1, fi) == 1) {
		for (int i = 0; i < len[c]; i++) {
			if (code[c][i] == '1')
				buf |= (1 << (7 - bit));
			if (++bit == 8) {
				fwrite(&buf, 1, 1, fo);
				buf = 0;
				bit = 0;
			}
		}
	}
	int valid_bits = bit;
	if (bit > 0) {
		fwrite(&buf, 1, 1, fo);
	}
	fwrite(&valid_bits, sizeof(int), 1, fo);
	
	*comp = ftell(fo);
	fclose(fi);
	fclose(fo);
	return 0;
}

int decompress_file(const char *in, const char *out) {
	FILE *fi = fopen(in, "rb");
	if (!fi) {
		perror("打开压缩文件失败");
		return -1;
	}
	FILE *fo = fopen(out, "wb");
	if (!fo) {
		perror("打开输出文件失败");
		fclose(fi);
		return -1;
	}
	
	int magic;
	fread(&magic, sizeof(int), 1, fi);
	if (magic != MAGIC) {
		printf("不是有效的压缩文件！\n");
		fclose(fi);
		fclose(fo);
		return -1;
	}
	
	int freq[MAX_SYM] = {0};
	fread(freq, sizeof(int), MAX_SYM, fi);
	
	HNode *root = build_tree(freq);
	HNode *cur = root;
	
	unsigned char c;
	int valid_bits;
	fseek(fi, sizeof(int), SEEK_END);
	fread(&valid_bits, sizeof(int), 1, fi);
	fseek(fi, sizeof(int) + sizeof(int) + MAX_SYM*sizeof(int), SEEK_SET);
	
	while (fread(&c, 1, 1, fi) == 1) {
		int bits = 8;
		if (feof(fi) && valid_bits > 0) {
			bits = valid_bits;
		}
		for (int i = 0; i < bits; i++) {
			cur = ((c >> (7 - i)) & 1) ? cur->right : cur->left;
			if (!cur->left && !cur->right) {
				fwrite(&cur->sym, 1, 1, fo);
				cur = root;
			}
		}
	}
	
	fclose(fi);
	fclose(fo);
	return 0;
}

int real_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		char op[10];
		char infile[256];
		char outfile[256];
		
		printf("===== 哈夫曼压缩/解压程序 =====\n");
		printf("请输入操作（c=压缩，d=解压）：");
		scanf("%9s", op);
		printf("请输入输入文件名：");
		scanf("%255s", infile);
		printf("请输入输出文件名：");
		scanf("%255s", outfile);
		
		char *new_argv[] = { argv[0], op, infile, outfile };
		return real_main(4, new_argv);
	}
	else
	{
		return real_main(argc, argv);
	}
}

int real_main(int argc, char *argv[]) {
	printf("程序启动！argc=%d\n", argc);
	fflush(stdout);
	
	if (argc != 4) {
		printf("Usage:\n  %s c input output\n  %s d input output\n",
			   argv[0], argv[0]);
		return 1;
	}
	
	if (!strcmp(argv[1], "c")) {
		long orig, comp;
		clock_t t1 = clock();
		
		int ret = compress_file(argv[2], argv[3], &orig, &comp);
		if (ret != 0) {
			printf("压缩失败！\n");
			return 1;
		}
		
		double ctime = (double)(clock() - t1) / CLOCKS_PER_SEC;
		
		printf("\n====== 压缩性能 ======\n");
		printf("原始大小 : %ld KB\n", orig / 1024);
		printf("压缩大小 : %ld KB\n", comp / 1024);
		printf("压缩比   : %.2f%%\n", (double)comp / orig * 100);
		printf("压缩时间 : %.3f s\n", ctime);
		
	} else if (!strcmp(argv[1], "d")) {
		clock_t t1 = clock();
		int ret = decompress_file(argv[2], argv[3]);
		if (ret != 0) {
			printf("解压失败！\n");
			return 1;
		}
		double dtime = (double)(clock() - t1) / CLOCKS_PER_SEC;
		
		printf("\n====== 解压性能 ======\n");
		printf("解压时间 : %.3f s\n", dtime);
	} else {
		printf("无效的操作！只能是 c 或 d\n");
		return 1;
	}
	
	system("pause"); 
	return 0;
}
