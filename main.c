#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#define COL 2000 //satir sayisi
#define ROW 2000 //sutun sayisi

typedef struct List {
    char path[100];
} List; //Dosya isimlerini tutan struct

typedef struct Text {
    char buff[2000];
} Text; //Dosyadan verileri okumak icin kullanilan struct

typedef struct SngTable {
    char unique_sng[2000][100];
    int unq_count;
} SngTable; //Shingle listelerini tutmak için kullanilan struct

List *files; //Dosya isimlerini tutan dizi
Text *T; // Dosyadan okunan verileri gecici tutan buffer
Text *data; // Dosyadan okunan ve filtre edilmis veriyi tutan dizi
FILE *fp[100]; //Dosyalari acmak icin kullanilan file pointerlari tutan dizi
SngTable *unique_table; // Her bir dosyanin unique shingle tablosunu tutan dizi
SngTable *merged_unque_table; // Tum dosyalardan gelen unique shingle tablosunu
float **jaccard_dist_matrix; // Unique shingle tablolarindan elde edilen jaccard matrisi
float **jaccard_by_signature_matrix; // Signature matristen elde edilen jaccard matrisi
int signature_matrix[ROW][COL]; // Signature matris
int map_matrix[ROW][COL]; // Hangi shingle'in hangi dokumanda olduğunu tutan matris
int hash_matrix[ROW][COL]; // Her shingle icin uretilen hash degerlerini tutan matris


void read_data(int); // Dosyalardan veri okuyan fonksiyon

void find_shingle(int, int); // Okunan veriden girilen K degerine gore shingle listelerini bulan fonksiyon

void jaccard_distance(int); // Bulunan shingle listelerine gore jaccard benzerligini hesaplayan fonksiyon

void merge_shingle_lists(int); // Tum dokumanlardan gelen shingle listelerini birlestiren fonksiyon

void print_shingle(int, int); // Shingle tablolarini ekrana yazdiran fonksiyon

void print_matrix(int, float **); // Jaccard matrislerini ekrana yazdiran fonksiyon

void fill_map_matrix(int); // Map matrisi dolduran fonksiyon

int find_closest_prime(int, int); // Toplam unique shingle sayisina en yakin asal sayiyi bulan fonksiyon

void find_signature_matrix(int, int); // Signature matrisi hesaplayan fonksiyon

void jaccard_by_signature(int); // Signature matrise gore jaccard benzerligini bulan fonksiyon

void print_document_pairs(int); // Girilen esik degerine gore benzerligi fazla olan dokumanlari yazdiran fonksiyon

int main() {
    int K = 100;
    int doc_number = 0;
    int M;

    printf("Isleme devam etmek icin K sayisini giriniz, cikmak icin '0' giriniz\n");
    while (K != 0) {
        printf("\nK===> ");
        scanf(" %d", &K);
        if (K == 0) {
            break;
        } else {
            if (doc_number == 0) {
                printf("Dokuman sayisini giriniz==>\n"); //kullanici dokuman sayisini girer
                scanf(" %d", &doc_number);
            }
            if (!files) { //Eger veri okunmamissa veriler dosyalardan okunur, K'nın degistigi durumlarda tekrar veri okunmaz
                read_data(doc_number); //Veriler dosyalardan okunur ve filtre edilir
            }
            find_shingle(K, doc_number); //Dosyalarin shingle listeleri olusturulur
        }
        printf("\nJaccard Benzerligi, K = %d\n", K);
        jaccard_distance(doc_number); // Dokumanlarin jaccard benzerligi ekrana yazdirilir
        merge_shingle_lists(doc_number); // Elde edilen shinle listeleri tek bir listede merge edilir
        fill_map_matrix(doc_number); // Eldeki shingle listelerine gore map matris doldurulur
        M = find_closest_prime(2,
                               merged_unque_table[0].unq_count); //Toplam unique shingle sayisina en yakin asal sayi bulunur
        find_signature_matrix(doc_number, M);//Signature matris elde edilir
        printf("\nImza Benzerligi, K = %d\n", K);
        jaccard_by_signature(doc_number); //Imza benzerlikleri bulunur ve ekrana yazdirilir.
        print_document_pairs(
                doc_number);//Girilen threshold degerine gore benzerligi fazla olan dokumanlar ekrana yazdirilir.
    }

    //Dinamik olarak ayrilan diziler boşaltilir.
    free(T);
    free(data);
    free(files);
    free(unique_table);
    free(merged_unque_table);
    free(jaccard_by_signature_matrix);
    free(jaccard_dist_matrix);
    return 0;
}

void jaccard_by_signature(int doc_number) {
    //imza matrisine gore dokuman benzerliklerini bulan fonksiyon
    int i; //indis
    int j; //indis
    int k; //indis
    float count; //imzalardaki benzerlik oranini tutan degisken
    if (doc_number == 0) {
        printf("\n;Dokuman sayisi '0' olamaz");
        return;
    } else {
        //benzerlik matrisi icin gerekli yer acilir
        jaccard_by_signature_matrix = calloc(doc_number, sizeof(float *));
        for (i = 0; i < doc_number; i++) {
            jaccard_by_signature_matrix[i] = calloc(doc_number, sizeof(float *));
            if (!jaccard_by_signature_matrix[i]) {
                printf("\nYer ayrilamadi");
                exit(-2);
            }
        }
    }

    //benzerlik matrisinin boyutlari documan sayisi kadardir
    for (i = 0; i < doc_number; i++) {
        for (j = i; j < doc_number; j++) {
            count = 0;
            for (k = 0; k < 100; k++) {
                //imza matrisinin sutunlarinda ilerlenir
                if (signature_matrix[k][i] == signature_matrix[k][j]) {
                    count++;
                }
            }
            //benzerlik oranlari ilgili gozlere yazilir
            jaccard_by_signature_matrix[i][j] = count / 100;
            jaccard_by_signature_matrix[j][i] = count / 100;
        }
    }
    printf("\n");
    //imza benzerligi ekrana yazdirilir
    print_matrix(doc_number, jaccard_by_signature_matrix);
}

void jaccard_distance(int doc_number) {
    int i; //indis
    int j; //indis
    int m; //indis
    int n; //indis
    int cmp; //karsilastirma sonucunu tutan degisken
    int same_sng; //dokumanlardaki ortak shingle sayisini tutan degisken
    int total_sng; //iki dokumandaki shingle birlesim kumesinin eleman sayisi
    float jaccard_similarity; //iki dokumanın jaccard benzerligini tutan matris
    if (doc_number == 0) {
        printf("\n;Dokuman sayisi '0' olamaz");
        return;
    } else {
        //jaccard matrisi icin gerekli yer ayriliyor
        jaccard_dist_matrix = calloc(doc_number, sizeof(float *));
        for (i = 0; i < doc_number; i++) {
            jaccard_dist_matrix[i] = calloc(doc_number, sizeof(float *));
            if (!jaccard_dist_matrix[i]) {
                printf("\nYer ayrilamadi");
                exit(-2);
            }
        }
    }

    //dokumanlarin shingle listeleri birbiriyle kıyaslanir, kesisim ve birlesim kumeleri bulunur
    for (i = 0; i < doc_number; i++) {
        for (j = i; j < doc_number; j++) {
            same_sng = 0; //kesisim sayisi '0' olarak baslatilir
            total_sng = 0; //birlesim sayisi '0' olarak baslatilir
            for (m = 0; m < unique_table[i].unq_count; m++) {
                for (n = 0; n < unique_table[j].unq_count; n++) {
                    cmp = strcmp(unique_table[i].unique_sng[m], unique_table[j].unique_sng[n]);
                    if (cmp == 0) { //eslesen shingle varsa kesisim arttirilir
                        same_sng++;
                    }
                }
            }
            //birlesim kümesi hesaplanir
            total_sng = total_sng + unique_table[i].unq_count + unique_table[j].unq_count;
            total_sng = total_sng - same_sng;
            //jaccard benzerlik orani hesaplanir ve matrisin ilgili gozune yazdirilir
            jaccard_similarity = (float) same_sng / (float) total_sng;
            jaccard_dist_matrix[i][j] = jaccard_similarity;
            jaccard_dist_matrix[j][i] = jaccard_similarity;

        }
    }

    printf("\n");
    //benzerlik matrisi ekrana yazdirilir
    print_matrix(doc_number, jaccard_dist_matrix);
}

void find_signature_matrix(int doc_number, int M) {
    int i; //indis
    int j; //indis
    int c; //indis
    int r; //indis
    int a; // random degisken

    srand(time(NULL)); //rastgele degisken uretmek icin kullanilan fonksiyon

    //imza matrisinin gozleri ilk basta cok büyük bir sayiyla doldurulur
    for (i = 0; i < 100; i++) {
        for (j = 0; j < doc_number; j++) {
            signature_matrix[i][j] = 9999;
        }
    }

    for (r = 0; r < merged_unque_table[0].unq_count; r++) { //toplam unique shingle sayisi kadar ilerlenir
        for (i = 0; i < 100; i++) { // hash matrisi random sayilarla doldurrulur
            a = rand() % M;
            hash_matrix[r][i] = (a * i + 1) % M;
        }
        for (c = 0; c < doc_number; c++) { //toplam dokuman sayisi kadar ilerlenir
            if (map_matrix[r][c] == 1) { //map matriste ilgili gozde '1' varsa signature matris doldurulmaya baslanir
                for (i = 0; i < 100; i++) { //Hash fonksiyonu sayisi kadar ileri gidilir
                    if (hash_matrix[r][i] <
                        signature_matrix[i][c]) { //uretilen hash fonksiyonlari arasindan en kucugu bulunur
                        signature_matrix[i][c] = hash_matrix[r][i]; //imza matrisinin ilgili gozu doldurulur
                    }
                }
            }
        }
    }
}

void fill_map_matrix(int doc_number) {
    //Hangi shinglein hangi dokumanda oldugunu gosteren matrisi bulan fonksiyondur
    //Matrisin gozleri daha onceden '0' ile doldurulmustu
    int i; //indis
    int j; //indis
    int m; //indis
    int cmp; // karsilastirma sonucunu tutan degisken

    for (i = 0; i < merged_unque_table[0].unq_count; i++) { //her dokumanin unique shingle sayisi kadar ilerlenir
        for (j = 0; j < doc_number; j++) { // dokumanlar arasinda ilerlenir
            for (m = 0;
                 m < unique_table[j].unq_count; m++) { //baktigimiz shingle o dokumanda varsa ilgili goz 1 yapilir
                cmp = strcmp(merged_unque_table[0].unique_sng[i], unique_table[j].unique_sng[m]);
                if (cmp == 0) {
                    map_matrix[i][j] = 1;
                    break;
                }
            }
        }
    }
}

int find_closest_prime(int low, int high) {
    int flag; //Asal sayiyi tutan degisken
    int i; //indis
    int max_prime; //M degerine yakin en buyuk asal sayiyi tutan degisken

    max_prime = 2;
    while (low < high) { //M degerine kadar ilerlenir ve asal sayilar kontrol edilir
        flag = 0;
        for (i = 2; i <= low / 2; i++) {
            if (low % i == 0) {
                flag = 1;
                break;
            }
        }
        if (flag == 0) {
            max_prime = low; // en buyuk asal sayi saklanir
        }
        low++;
    }
    return max_prime; // en buyuk asal sayi geri dondurulur
}

void print_matrix(int doc_number, float **matrix) {
    int i;  //indis
    int j; //indis

    if (doc_number == 0) {
        printf("\n;Dokuman sayisi '0' olamaz");
        return;
    }

    // Elde edilen jaccard matrisleri istenen formatta ekrana yazdirilir
    printf("    ");
    for (i = 0; i < doc_number / 2; i++) {
        printf("%d    ", i + 1);
    }
    for (i = doc_number / 2; i < doc_number; i++) {
        printf("%d   ", i + 1);
    }
    printf("\n--------------------------------------------------------------------------------------------------------\n");
    for (i = 0; i < doc_number; i++) {
        printf("%d| ", i + 1);
        for (j = 0; j < doc_number; j++) {
            printf("%.2f|", matrix[i][j]);
        }
        printf("\n\n");
    }
}

void merge_shingle_lists(int doc_number) {
    int i;  //indis
    int j; //indis
    int m; //indis
    int cmp; //karsilastirma icin kullanilan gecici degisken
    char C; //kullanicinin secim yapmasi icin kullanilan gecici degisken

    //Unique shingle tablosu icin gerekli yer acilir
    merged_unque_table = calloc(1, sizeof(struct SngTable));
    if (!merged_unque_table) {
        printf("\nYer ayrilamadi");
        exit(-3);
    }

    cmp = 1;

    //unique tablonun sayaci 0 olarak ayarlanir
    merged_unque_table[0].unq_count = 0;

    //Her dokumandan gelen shingle daha once elde edilmis mi diye kontrol edilir
    for (i = 0; i < doc_number; i++) {
        for (j = 0; j < unique_table[i].unq_count; j++) {
            for (m = 0; m < merged_unque_table[0].unq_count; m++) {
                //eger shingle daha once alinmissa tekrar eklenmez
                cmp = strcmp(unique_table[i].unique_sng[j], merged_unque_table[0].unique_sng[m]);
                if (cmp == 0) {
                    break;
                }
            }
            if (cmp != 0) {//gelen shingle unique ise tabloya eklenir
                strcpy(merged_unque_table[0].unique_sng[merged_unque_table[0].unq_count],
                       unique_table[i].unique_sng[j]);
                merged_unque_table[0].unq_count++;
            }
        }
    }

    //Unique shingle tablosu ihtiyaca gore ekrana yazdirilir
    printf("\n\n========TOPLAM UNIQUE SHINGLE SAYISI=>%d\n\n", merged_unque_table[0].unq_count);
    printf("\nMerge edilmis unique shingle tablosu yazilsin mi? e/E/h/H");
    scanf(" %c", &C);

    if (C == 'e' || C == 'E') {
        printf("\nMerged unique shingle tablos\n");
        for (i = 0; i < merged_unque_table[0].unq_count; i++) {
            printf("%s\t", merged_unque_table[0].unique_sng[i]);
        }
        printf("\n");
    }
    printf("\n");
}

void find_shingle(int K, int doc_number) {
    int i; //indis
    int u; //indis
    int j; //indis
    int is_unq; //shinglein unique olup olmadigini tutan degisken
    char *tmp; //shinglelari gecici olarak tutan degisken
    char C; // Kullanicinin secim yapmasini saglayan degisken

    if (doc_number == 0) {
        printf("Dokuman sayisi '0' olamaz");
        return;
    }

    //dokuman sayisi kadar tablo acilir
    unique_table = (struct SngTable *) malloc(doc_number * sizeof(struct SngTable));
    tmp = (char *) calloc(100, sizeof(char));
    if (!unique_table || !tmp) {
        printf("Yer ayrilamadi");
        exit(2);
    }

    //girilen K sayisina gore shinglelar bulunur ve tablolara yazdirilir
    for (i = 0; i < doc_number; i++) {
        unique_table[i].unq_count = 0; // her dosyanin unique shingle sayisi tutulur
        is_unq = 100;
        for (u = 0; u < strlen(data[i].buff) - K + 1; u++) {//shinglelar elde edilir
            for (j = 0; j < K; j++) {
                tmp[j] = data[i].buff[u + j];
            }
            for (j = 0; j < u; j++) { //elde edilen shingle unique mi diye kontrol edilir
                is_unq = strcmp(unique_table[i].unique_sng[j], tmp);
                if (is_unq == 0) {
                    break;
                }
            }
            if (is_unq != 0) { //eger elde edilen shingle unique is tabloya eklenir
                strcpy(unique_table[i].unique_sng[unique_table[i].unq_count], tmp);
                unique_table[i].unq_count++;
            }
        }
    }

    printf("\n=======================================================\n");
    printf("K = %d icin dokumanlarin unique shingle sayilari ==>\n", K);
    for (i = 0; i < doc_number; i++) {
        printf("\nDokuman => %d, unique shingle =>%d", i + 1, unique_table[i].unq_count);
    }

    printf("\nShingle tablolari yazilsin mi ? e/E/h/H");
    scanf(" %c", &C);
    if (C == 'e' || C == 'E') {
        print_shingle(K, doc_number);
    }
    printf("\n\n");
}

void print_shingle(int K, int doc_number) {
    int i; //indis
    int j; //indis

    //Her dokumanin unique shingle listesi ekrana bastirilir
    printf("\nUnique Shingles ==>\n");
    for (i = 0; i < doc_number; i++) {
        printf("\n==========================================================================================");
        printf("\nDokuman ==>%d\n============================================================================\n",
               i + 1);
        for (j = 0; j <= unique_table[i].unq_count; j++) {
            printf("%s\t", unique_table[i].unique_sng[j]);
        }
    }
    printf("\n\n");
}

void read_data(int doc_number) {
    int i; //indis
    int j; //indis
    int k; //indis
    char tmp[50]; // Kelimelerin okundugu gecici degisken
    char space[2] = " "; // Her okunan kelimeden sonra bir bosluk kelimenin sonuna eklenir
    long fsize; // Dosyalarin boyutunu tutan degisken

    if (doc_number == 0) {
        printf("Dokuman sayisi '0' olamaz");
        return;
    }

    //Kullanilacak olan diziler diziler ve veriler icin yer ayrilir
    files = (struct List *) malloc(doc_number * sizeof(struct List));
    T = (struct Text *) malloc(doc_number * sizeof(struct Text));
    data = (struct Text *) malloc(doc_number * sizeof(struct Text));

    if (!files || !T || !data) {
        printf("yer ayrilamadi");
        exit(-1);
    }

    //dosya isimleri dinamik olarak ayarlanip diziye atilir
    for (i = 1; i <= doc_number; i++) {
        sprintf(files[i - 1].path, "%d.txt", i);
    }

    //her dosya icin ayri bir file pointer tutulur ve dosyalar acilir.
    for (i = 0; i < doc_number; i++) {
        fp[i] = fopen(files[i].path, "r"); // dosyalar sirayla acilir
        if (!fp[i]) {
            printf("Dosya %d acilamadi", i + 1);
            exit(1);
        }

        fseek(fp[i], 0, SEEK_END);  //acilan dosyanin boyutu bulunur
        fsize = ftell(fp[i]);
        rewind(fp[i]); // pointer dosyanin basina alinir

        //Veriler dosyadan kelime kelime okunur ve sonlarina bir bosluk eklenerek gecici buffera atilir
        while (!feof(fp[i])) {
            fscanf(fp[i], "%s", tmp);
            strcat(T[i].buff, tmp);
            strcat(T[i].buff, space);
        }

        j = 0;

        //Gecici olarak buffera alinan veriler bosluk ve harf karakterleri icin filtre edilir
        for (k = 0; k <= fsize; k++) {
            if (isalpha(T[i].buff[k]) || T[i].buff[k] == ' ') {
                data[i].buff[j] = T[i].buff[k];
                j++;
            }
        }

        strlwr(data[i].buff); //filtre edilmis veri kucuk harfe cevrilir ve saklanir

    }

    //acilan butun dosyalar kapatilir
    for (i = 0; i < doc_number; i++) {
        fclose(fp[i]);
    }
}

void print_document_pairs(int doc_number) {
    float threshold[3] = {0.7, 0.8, 0.9}; //Esik degeri
    int i; //indis
    int j; //indis
    int thr; //indis

    printf("\nK-SHINGLE BENZERLIKLERINE GORE DOKUMAN CIFTLERI");
    for (thr = 0; thr < 3; thr++) {
        printf("\n----------------------------------------------------------------------------------------------------");
        printf("\nThreshold %0.2f icin Benzerlik orani esik degerinden fazla olan dokuman ciftleri==>\n\n",
               threshold[thr]);
        for (i = 0; i < doc_number - 1; i++) {
            for (j = i + 1; j < doc_number; j++) {
                if (jaccard_dist_matrix[i][j] >= threshold[thr]) {
                    //Diyagonalden ustteki degerlere bakmak yeterlidir.
                    printf("dokuman(%d, %d) benzerlik orani=> %0.2f\t", i + 1, j + 1, jaccard_dist_matrix[i][j]);
                }
            }
        }
    }
    printf("\n----------------------------------------------------------------------------------------------------");
    printf("\n----------------------------------------------------------------------------------------------------");
    printf("\nIMZA BENZERLIKLERINE GORE DOKUMAN CIFTLERI");
    for (thr = 0; thr < 3; thr++) {
        printf("\n-----------------------------------------------------------------------------------------------------");
        printf("\nThreshold %0.2f icin Benzerlik orani esik degerinden fazla olan dokuman ciftleri==>\n\n",
               threshold[thr]);
        for (i = 0; i < doc_number - 1; i++) {
            for (j = i + 1; j < doc_number; j++) {
                if (jaccard_by_signature_matrix[i][j] >= threshold[thr]) {
                    //Diyagonalden ustteki degerlere bakmak yeterlidir.
                    printf("dokuman(%d, %d)imza benzerligi=> %0.2f\t", i + 1, j + 1, jaccard_by_signature_matrix[i][j]);
                }
            }
        }
    }
    printf("\n");
}



