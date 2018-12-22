#include<stdio.h>
#include<stdlib.h>

int main(int argc, char *argv[]){
	FILE *fp;	//ファイルポインタ作成
	unsigned char bitmapImage[2][32];	//ビットマップ画像格納先
	unsigned char bitmapFileHeader[14];		//ビットマップファイルヘッダ保存先
	unsigned long Size;	//情報ヘッダサイズ保存先
	long width, height;	//幅、高さ保存先
	unsigned char bitmapInfoHeaderOther[28]; //ビットマップ情報ヘッダ 残りのデータ保存先
	unsigned char bitmapPalette[8];		//ビットマップパレットデータ保存先
	unsigned char bitmapDatafield[2][32];	//画像データの保存先
	unsigned char oledDatafield[16][4];		//ビットマップからOLED用に変換したデータの保存先
	int i, j, x, y;	//forループ用変数
	unsigned char count, dummy[2];	//byte空読み用
	printf("引数の数は%dです\n", argc);
	if (argc != 3){
		printf("引数が正しくありません。\n");
		printf("引数1:16×32 モノクロビットマップ, 引数2:出力先フォルダ\\ファイル名.txt\n");
		exit(EXIT_FAILURE);
	}
	if (fopen_s(&fp ,argv[1], "rb") != 0){
		printf("画像ファイルが開けませんでした\n");
		exit(EXIT_FAILURE);
	}
	fread(bitmapFileHeader, sizeof(char), 14, fp);	//ファイルヘッダの読み込み
	fread(&Size, sizeof(long), 1, fp);	//ヘッダサイズの読み込み
	fread(&width, sizeof(long), 1, fp);	//画像幅の読み込み
	fread(&height, sizeof(long), 1, fp);	//画像高さの読み込み
	fread(bitmapInfoHeaderOther, sizeof(char), 28, fp);	//情報ヘッダの読み込み
	fread(bitmapPalette, sizeof(char), 8, fp);	//パレットの読み込み
	printf("bitmapの幅：%d, 高さ：%d\n", width, height);
	if ((width != 16) | (height != 32)){	//画像サイズをチェック
		printf("画像サイズが違います。\n");
		printf("適正サイズ：16×32\n");
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	//画像開始ポイントまで空読み
	if (bitmapFileHeader[10] > 62){	//ファイルヘッダのbfOffBitsの最下位byteの値を確認
		count = bitmapFileHeader[13] - 62;
		for (i = 0; i < count; i++){	//bfOffBits指定位置まで空読みする
			fread(dummy, sizeof(char), 1, fp);
		}
	}
	//bitmapのデータを左下順から左上順に並べ替えて取得
	for (j = 0; j < 32; j++){
		for (i = 0; i < 2; i++){
			fread(&bitmapDatafield[i][31 - j], sizeof(char), 1, fp);
		}
		fread(dummy, sizeof(char), 2, fp);	//bitmapの4byte(long)境界に合わせて空読み
	}
	//bitmapのデータをoled用データ列に変換
	for (j = 0; j < 4; j++){
		for (i = 0; i < 2; i++){
			//8bit × 8行 の行列空間ごとに分けてデータの変換を行う
			for (y = 0; y < 8; y++){
				for (x = 0; x < 8; x++){
					if (y == 0)oledDatafield[i * 8 + x][j] = 0;	//初めに初期化
					oledDatafield[i * 8 + x][j] |= 
						(char)(((bitmapDatafield[i][j * 8 + y] >> (7 - x)) & 0x0001) << y);
				}
			}
		}
	}
	fclose(fp);
	if (fopen_s(&fp, argv[2], "wb") != 0){
		printf("txtファイルが開けませんでした\n");
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	//txtファイルに書き込み
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
	printf("完了\n");
	exit(EXIT_SUCCESS);
	return 0;
}