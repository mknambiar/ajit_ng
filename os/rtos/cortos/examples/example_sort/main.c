#include "cortos.h"

#define SIZE 20

char arr[SIZE] = {
  182, 207, 224, 124, 143, 49, 242, 199, 68, 124, 33, 245, 37, 225, 178, 96, 178, 105, 149, 220
};

void quicksort(char arr[], int first, int last){
   int i, j, pivot, temp;

   if(first < last){
      pivot = first;
      i = first;
      j = last;

      while(i < j){
         while(arr[i] <= arr[pivot] && i < last)
            i++;
         while(arr[j] > arr[pivot])
            j--;
         if(i<j){
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
         }
      }

      temp = arr[pivot];
      arr[pivot] = arr[j];
      arr[j] = temp;

      quicksort(arr,first,j-1);
      quicksort(arr,j+1,last);

   }
}

void mergesort(char arr[]) {
}

int main() {
  int step = SIZE / CORTOS_THREADS;
  char tid = cortos_get_thread_id();
  int start = step*tid;

  quicksort(arr, start, start+step-1);

  // merge different segments of sorted elements.

  // terminate the thread cleanly.
  cortos_exit(0);
}
