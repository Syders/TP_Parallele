#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <sys/time.h>


#define MAX_TEXT_LENGTH 100000
#define MAX_KEY_LENGTH 20
#define NUM_THREAD 1

#define PF_NUMBER 8
int prime_factors[PF_NUMBER] = {2,3,5,7,11,13,17,19};

// ------------------------------------------------------
char * readText(char *filename){;
  FILE *f = fopen(filename,"r");
  if (f == NULL) {
    fprintf(stderr,"File %s not found...\n",filename);
    return "\0";
  }
  int i = 0;
  char *str = (char *) malloc(MAX_TEXT_LENGTH * sizeof(char));
  while ((i<MAX_TEXT_LENGTH-1) && (fread(&str[i],1,sizeof(char),f)))
    i++;
  str[i] = 0;
  fclose(f);
  return str;
}

// ------------------------------------------------------
int encodePrimeFactorization(int number){
  int code = 0;
  for (int i=PF_NUMBER-1; i>=0 ; i--){
    code = code * 2;
    int f = prime_factors[i];
    if (number % f == 0){
      code = code + 1;
    }
  }
  return code;
}

// ------------------------------------------------------
int decodePrimeFactorization(int code){
  int prod = 1;
  for (int j=0; j<PF_NUMBER ; j++){
    if ((code & 1) == 1){
      prod = prod * prime_factors[j];
    }
    code = code / 2;
  }
  return(prod);
}

// ------------------------------------------------------
int computeKeyLength(char *text){
  int length = strlen(text);
  int *num_facts = malloc((1<<PF_NUMBER) * sizeof(int));
  int max_num_facts = 0;
  int most_frequent_fact;
  //parallelisable mais inefficace
  for (int i=0; i<(1<<PF_NUMBER) ; i++)
        num_facts[i] = 0;
#pragma omp parallel num_threads(NUM_THREAD)
  {
    #pragma omp for schedule(dynamic)
    //parallelisable
    for (int i=0; i<length; i++){
        //for avec pas par default
        for (int j=i+1; j<length; j++){
            if (text[i] == text[j]){
                int k = 1;
                while (text[i+k] == text[j+k])k++;
                if (k >= 3){
                    int fact = encodePrimeFactorization(j-i);
                    num_facts[fact] ++;
                    break;
                }
            }
        }
    }
  }
  //parallelisable mais perte de temps
  for (int i=0; i<(1<<PF_NUMBER) ; i++){
      if (num_facts[i] > max_num_facts){
          max_num_facts = num_facts[i];
          most_frequent_fact = i;
      }
  }
  free(num_facts);
  int key_length = decodePrimeFactorization(most_frequent_fact);
  return(key_length);
}

// ------------------------------------------------------
char *computeKey(int key_length, char *text){
  char *key = (char*) malloc((key_length+1) * sizeof(char));
  int text_length = strlen(text);
  int **histogram = (int **) malloc(key_length * sizeof(int *));
  //parallelisable mais inefficace
  for (int i=0; i<key_length ; i++){
      histogram[i] = malloc(26 * sizeof(int));
      for (int j=0; j<26 ; j++)
          histogram[i][j] = 0;
  }
#pragma omp parallel num_threads(NUM_THREAD)
  {
    //parallelisable
    #pragma omp for
    for (int i=0; i<key_length; i++){
        for (int j=i; j<text_length ; j+=key_length){
            histogram[i][text[j]-'A']++;
        }
        int max = 0;
        int most_frequent_letter;
        for (int j=0; j<26 ; j++){
            if (histogram[i][j] > max){
                max = histogram[i][j];
                most_frequent_letter = j;
            }
        }
        key[i] = (char) (((most_frequent_letter - ('E'-'A') + 26) % 26) + 'A') ;
    }
    key[key_length] = 0;
  }
    //parallelisable mais inefficace
    for (int i=0; i<key_length ; i++)
        free(histogram[i]);
  
  free(histogram);
  return(key);
}


// ------------------------------------------------------
char *decipher(char *ciphertext, char *key){
  int text_length = strlen(ciphertext);
  int key_length = strlen(key);

  char * cleartext = malloc(text_length * sizeof(char));
  int j = 0;
  //parallelisable  portion de long key_length
  #pragma omp parallel for schedule(static,key_length) private(j) num_threads(NUM_THREAD)
  for (int i=0; i < text_length; i++){
    cleartext[i] = ((ciphertext[i] -'A' - key[j] + 'A' + 26) % 26) + 'A';
    j = (j+1) % key_length;
  }
  return cleartext;
}

// ------------------------------------------------------
int main(int argc, char **argv) {
    if (argc!=2){
      printf("Usage : vigenere <input file>\n\n");
      return 1;
    }

    char *ciphertext = readText(argv[1]);
    printf("---- Ciphertext ----\n");
    printf("%s\n\n",ciphertext);

    printf("---- Key ----\n");
    int key_length = computeKeyLength(ciphertext);
    char *key = computeKey(key_length,ciphertext);
    printf("%s\n\n",key);

    char *cleartext = decipher(ciphertext, key);
    printf("---- Cleartext ----\n");
    printf("%s\n\n",cleartext);

  free(ciphertext);
  free(key);
  free(cleartext);
  return 0;
}
