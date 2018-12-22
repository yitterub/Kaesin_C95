#include<stdio.h>
#include<stdlib.h>

int main(int argc, char *argv[]){
	FILE *fp;	//�t�@�C���|�C���^�쐬
	unsigned char bitmapImage[2][32];	//�r�b�g�}�b�v�摜�i�[��
	unsigned char bitmapFileHeader[14];		//�r�b�g�}�b�v�t�@�C���w�b�_�ۑ���
	unsigned long Size;	//���w�b�_�T�C�Y�ۑ���
	long width, height;	//���A�����ۑ���
	unsigned char bitmapInfoHeaderOther[28]; //�r�b�g�}�b�v���w�b�_ �c��̃f�[�^�ۑ���
	unsigned char bitmapPalette[8];		//�r�b�g�}�b�v�p���b�g�f�[�^�ۑ���
	unsigned char bitmapDatafield[2][32];	//�摜�f�[�^�̕ۑ���
	unsigned char oledDatafield[16][4];		//�r�b�g�}�b�v����OLED�p�ɕϊ������f�[�^�̕ۑ���
	int i, j, x, y;	//for���[�v�p�ϐ�
	unsigned char count, dummy[2];	//byte��ǂݗp
	printf("�����̐���%d�ł�\n", argc);
	if (argc != 3){
		printf("����������������܂���B\n");
		printf("����1:16�~32 ���m�N���r�b�g�}�b�v, ����2:�o�͐�t�H���_\\�t�@�C����.txt\n");
		exit(EXIT_FAILURE);
	}
	if (fopen_s(&fp ,argv[1], "rb") != 0){
		printf("�摜�t�@�C�����J���܂���ł���\n");
		exit(EXIT_FAILURE);
	}
	fread(bitmapFileHeader, sizeof(char), 14, fp);	//�t�@�C���w�b�_�̓ǂݍ���
	fread(&Size, sizeof(long), 1, fp);	//�w�b�_�T�C�Y�̓ǂݍ���
	fread(&width, sizeof(long), 1, fp);	//�摜���̓ǂݍ���
	fread(&height, sizeof(long), 1, fp);	//�摜�����̓ǂݍ���
	fread(bitmapInfoHeaderOther, sizeof(char), 28, fp);	//���w�b�_�̓ǂݍ���
	fread(bitmapPalette, sizeof(char), 8, fp);	//�p���b�g�̓ǂݍ���
	printf("bitmap�̕��F%d, �����F%d\n", width, height);
	if ((width != 16) | (height != 32)){	//�摜�T�C�Y���`�F�b�N
		printf("�摜�T�C�Y���Ⴂ�܂��B\n");
		printf("�K���T�C�Y�F16�~32\n");
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	//�摜�J�n�|�C���g�܂ŋ�ǂ�
	if (bitmapFileHeader[10] > 62){	//�t�@�C���w�b�_��bfOffBits�̍ŉ���byte�̒l���m�F
		count = bitmapFileHeader[13] - 62;
		for (i = 0; i < count; i++){	//bfOffBits�w��ʒu�܂ŋ�ǂ݂���
			fread(dummy, sizeof(char), 1, fp);
		}
	}
	//bitmap�̃f�[�^�����������獶�㏇�ɕ��בւ��Ď擾
	for (j = 0; j < 32; j++){
		for (i = 0; i < 2; i++){
			fread(&bitmapDatafield[i][31 - j], sizeof(char), 1, fp);
		}
		fread(dummy, sizeof(char), 2, fp);	//bitmap��4byte(long)���E�ɍ��킹�ċ�ǂ�
	}
	//bitmap�̃f�[�^��oled�p�f�[�^��ɕϊ�
	for (j = 0; j < 4; j++){
		for (i = 0; i < 2; i++){
			//8bit �~ 8�s �̍s���Ԃ��Ƃɕ����ăf�[�^�̕ϊ����s��
			for (y = 0; y < 8; y++){
				for (x = 0; x < 8; x++){
					if (y == 0)oledDatafield[i * 8 + x][j] = 0;	//���߂ɏ�����
					oledDatafield[i * 8 + x][j] |= 
						(char)(((bitmapDatafield[i][j * 8 + y] >> (7 - x)) & 0x0001) << y);
				}
			}
		}
	}
	fclose(fp);
	if (fopen_s(&fp, argv[2], "wb") != 0){
		printf("txt�t�@�C�����J���܂���ł���\n");
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	//txt�t�@�C���ɏ�������
	fprintf(fp, "\tconst unsigned char pixelData[4][16] = {\r\n\t");
	for (j = 0; j < 4; j++){
		fprintf(fp, "{ ");
		for (i = 0; i < 16; i++){
			fprintf(fp, "0x%x", oledDatafield[i][j]);
			if (i < 15)fprintf(fp, " ,");
		}
		if(j < 3)fprintf(fp, "},\r\n\t");
		else fprintf(fp, "}\r\n\t};");
	}
	fclose(fp);
	printf("����\n");
	exit(EXIT_SUCCESS);
	return 0;
}