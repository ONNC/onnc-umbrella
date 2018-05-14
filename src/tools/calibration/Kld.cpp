//===- Kld.cpp ------------------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// TODO: Fix coding style...
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>

#define MULTI_THREAD_KL_CALC
#ifdef MULTI_THREAD_KL_CALC
#include <ctime>
#include <pthread.h>
#endif

namespace onnc {

#ifdef MULTI_THREAD_KL_CALC
struct mul_thread_inputs {
  float *m_Hist;
  float *m_KL;
  int m_Count;
  int m_I;
  int m_N;
  int m_Bins;
};
#endif

static inline void print_trace()
{
  void *array[10];
  size_t i;

  size_t size = backtrace(array, 10);
  char **strings = backtrace_symbols(array, size);

  printf("Obtained %lu stack frames.\n", size);

  for (i = 0; i < size; i++)
    printf("%s\n", strings[i]);

  free(strings);
}

static inline void hang(int pRet) { exit(pRet); }

#define ASSERT(_cond)                                                          \
  do {                                                                         \
    if (!(_cond)) {                                                            \
      printf("ASSERT %s: %d: %s: %s\n", __FILE__, __LINE__, __func__, #_cond); \
      print_trace();                                                           \
      fflush(stdout);                                                          \
      hang(-1);                                                                \
    }                                                                          \
  } while (0)

inline float the_max(float *pData, int pCount)
{
  ASSERT(pCount > 0);
  ASSERT(pData != nullptr);
  float a_max = fabs(pData[0]);
  for (int i = 1; i < pCount; i++) {
    a_max = (a_max < fabs(pData[i])) ? fabs(pData[i]) : a_max;
  }
  return a_max;
}

inline int the_min_index(float *pData, int pCount)
{
  ASSERT(pData != nullptr);
  float a_max = pData[0];
  int min_index = 0;
  for (int i = 1; i < pCount; i++) {
    if (a_max > pData[i]) {
      a_max = pData[i];
      min_index = i;
    }
  }

  return min_index;
}

float real_kl_diversity(float *pData, int pCount)
{
  const int N = 2048;
  const int BINS = 128;
  const int KL_NUM = N / BINS;
  float hist[N] = { 0.0 };
  float kl[KL_NUM] = { 0.0 };

  float data_max = the_max(pData, pCount);
  float width = data_max / (N - 1);

  printf("%s %d: data_max=%f width=%f\n", __func__, __LINE__, data_max, width);

  for (int i = 0; i < pCount; i++) {
    int index = floor(fabs(pData[i]) / width + 0.5);
    hist[index] += 1;
  }

  int m = 0;
  float sum = 0;
  for (int i = BINS; i < N + 1; i += BINS) {
    // P distribution
    float *P = (float *)malloc(sizeof(float) * i);
    for (int j = 0; j < i; j++) {
      P[j] = 0;
    }
    for (int j = 0; j < N; j++) {
      int index = (j > (i - 1)) ? (i - 1) : j;
      P[index] += hist[j];
    }
    for (int j = 0; j < i; j++) {
      P[j] /= pCount;
    }

    // Q distribution
    int expand_size = i / BINS;
    int idx = 0;
    float *Q = (float *)malloc(sizeof(float) * i);
    for (int j = i - BINS; j < i; j++) {
      sum += hist[j];
    }
    for (int j = 0; j < BINS; j++) {
      float sum_bin = 0;
      float positive_cnt = 0;
      int bin_idx = idx;
      for (int k = 0; k < expand_size; k++) {
        sum_bin += hist[idx];
        positive_cnt += (hist[idx] > 0) ? 1 : 0;
        idx++;
      }
      positive_cnt = (positive_cnt == 0) ? 1 : positive_cnt;
      float Q_base = sum_bin / positive_cnt / sum;
      while (bin_idx < idx) {
        Q[bin_idx] = hist[bin_idx] ? Q_base : 0;
        bin_idx++;
      }
    }
    for (idx = 0; idx < i; idx++) {
      kl[m] += P[idx] * (log10(P[idx] + 1e-30) - log10(Q[idx] + 1e-30));
    }
    m++;

    free(P);
    free(Q);
  }

  int m_min = the_min_index(kl, m);
  float threshold = width * (m_min + 1) * BINS;
  printf("  threshold: %.12f, m: %d, kl: %f\n", threshold, m_min, kl[m_min]);

  return threshold;
}

#ifdef MULTI_THREAD_KL_CALC

void *kl_calc_thread(void *pArgsInput)
{
  struct mul_thread_inputs *args;
  args = (struct mul_thread_inputs *)pArgsInput;
  float *hist = args->m_Hist;
  int i = args->m_I;
  int count = args->m_Count;
  int N = args->m_N;
  int BINS = args->m_Bins;

  // P distribution
  float *P = (float *)malloc(sizeof(float) * i);
  for (int j = 0; j < i; j++) {
    P[j] = 0;
  }
  for (int j = 0; j < N; j++) {
    int index = (j > (i - 1)) ? (i - 1) : j;
    P[index] += hist[j];
  }
  for (int j = 0; j < i; j++) {
    P[j] /= count;
  }

  // Q distribution
  float sum = 0.0;
  int expand_size = i / BINS;
  int idx = 0;
  float *Q = (float *)malloc(sizeof(float) * i);
  for (int j = 0; j < i; j++) {
    sum += hist[j];
  }
  for (int j = 0; j < BINS; j++) {
    float sum_bin = 0;
    float positive_cnt = 0;
    int bin_idx = idx;
    for (int k = 0; k < expand_size; k++) {
      sum_bin += hist[idx];
      positive_cnt += (hist[idx] > 0) ? 1 : 0;
      idx++;
    }
    positive_cnt = (positive_cnt == 0) ? 1 : positive_cnt;
    float Q_base = sum_bin / positive_cnt / sum;
    while (bin_idx < idx) {
      Q[bin_idx] = hist[bin_idx] ? Q_base : 0;
      bin_idx++;
    }
  }
  for (idx = 0; idx < i; idx++) {
    *(args->m_KL) += P[idx] * (log10(P[idx] + 1e-30) - log10(Q[idx] + 1e-30));
  }

  free(P);
  free(Q);

  return nullptr;
}

float real_multi_thread_kl_diversity(float *pData, int pCount)
{
  const int N = 2048;
  const int BINS = 128;
  const int KL_NUM = N / BINS;
  float hist[N] = { 0.0 };
  float kl[KL_NUM] = { 0.0 };

  float data_max = the_max(pData, pCount);
  float width = data_max / (N - 1);

  printf("%s %d: data_max=%f width=%f\n", __func__, __LINE__, data_max, width);

  for (int i = 0; i < pCount; i++) {
    int index = floor(fabs(pData[i]) / width + 0.5);
    hist[index] += 1;
  }

  int m = 0;
  pthread_t id[KL_NUM];
  struct mul_thread_inputs args_input[KL_NUM];
  for (int i = BINS; i < N + 1; i += BINS) {
    args_input[m].m_Hist = hist;
    args_input[m].m_KL = &kl[m];
    args_input[m].m_Count = pCount;
    args_input[m].m_I = i;
    args_input[m].m_N = N;
    args_input[m].m_Bins = BINS;
    int ret =
        pthread_create(&id[m], nullptr, kl_calc_thread, (void *)&args_input[m]);
    if (ret) {
      printf("Create No. %d thread error!\n", m);
      exit(1);
    }
    m++;
  }

  for (int i = 0; i < m; i++) {
    pthread_join(id[i], nullptr);
  }

  int m_min = the_min_index(kl, m);
  float threshold = width * (m_min + 1) * BINS;
  printf("  threshold: %f, m: %d, kl: %f\n", threshold, m_min, kl[m_min]);

  return threshold;
}
#endif

float KLDiversity(float *pData, int pCount)
{
#ifdef MULTI_THREAD_KL_CALC
  return real_multi_thread_kl_diversity(pData, pCount);
#else
  return real_kl_diversity(data, count);
#endif
}

} // namespace onnc
