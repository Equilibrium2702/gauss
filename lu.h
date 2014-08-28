#include <functional>
#include <immintrin.h>

//faster but not determined
//Matrix size must be >=1
template<class Matrix>
void simpleLU(Matrix& luMatrix, const int size)
{
  for (size_t i = 0; i < size; i++) {
    for (size_t k = 0; k < i; k++)
      for (size_t j = i; j < size; j++)
        luMatrix[i][j] -= luMatrix[i][k] * luMatrix[k][j];

    for (size_t j = i + 1; j < size; j++) {
      decltype(luMatrix[0][0] + luMatrix[0][0]) sum = 0;
      for (size_t k = 0; k < i; k++)
        sum += luMatrix[j][k] * luMatrix[k][i];
      luMatrix[j][i] = (luMatrix[j][i] - sum) / luMatrix[i][i];
    }
  }
}

// faster but not determined
template<class Matrix>
void blockLU(Matrix& luMatrix, const int size, const int bs)
{
  const int bn = (size + (bs - 1)) / bs;

  std::function<int(int)> lBorder = [&](int blockn) {
    return blockn * bs;
  };
  std::function<int(int)> rBorder = [&](int blockn) {
    return (blockn + 1) * bs > size ? size : (blockn + 1) * bs;
  };

  for (int i = 0; i < bn; i++) {
    // middle blocks
    //  full U and L part
    for (int k = 0; k < i; k++)
      for (int ii = lBorder(i), rbi = rBorder(i); ii < rbi; ii++)
        for (int ik = lBorder(k), rbk = rBorder(k); ik < rbk; ik++)
          for (int ij = lBorder(i); ij < rbi; ij++)
            luMatrix[ii][ij] -= luMatrix[ii][ik] * luMatrix[ik][ij];
    // U and L part last block
    for (int ii = lBorder(i), rbi = rBorder(i); ii < rbi; ii++) {
      for (int ik = lBorder(i); ik < ii; ik++) {
        for (int ij = ii; ij < rbi; ij++)
          luMatrix[ii][ij] -= luMatrix[ii][ik] * luMatrix[ik][ij];
        for (int ij = ii + 1; ij < rbi; ij++)
          luMatrix[ij][ii] -= luMatrix[ij][ik] * luMatrix[ik][ii];
      }
      //  end of L part
      for (int ij = ii + 1, rbi = rBorder(i); ij < rbi; ij++)
        luMatrix[ij][ii] /= luMatrix[ii][ii];
    }

    //U blocks
    for (int j = i + 1; j < bn; j++) {
      __m256d ymm0_ii_ik;
      __m256d ymm1_ii_ij;
      __m256d ymm2_ik_ij;
      for (int k = 0; k < i; k++)
        for (int ii = lBorder(i), rbi = rBorder(i); ii < rbi; ii++)
          for (int ik = lBorder(k), rbk = rBorder(k); ik < rbk; ik++)
          {
            ymm0_ii_ik = _mm256_broadcast_sd( &luMatrix[ii][ik] );
            for (int ij = lBorder(j), rbj = rBorder(j); ij < rbj; ij+=4)
            {
                ymm1_ii_ij = _mm256_loadu_pd( &luMatrix[ii][ij] );
                ymm2_ik_ij = _mm256_loadu_pd( &luMatrix[ik][ij] );
                ymm2_ik_ij = _mm256_mul_pd( ymm0_ii_ik, ymm2_ik_ij );
                ymm1_ii_ij = _mm256_sub_pd( ymm1_ii_ij, ymm2_ik_ij );
                _mm256_storeu_pd(&luMatrix[ii][ij], ymm1_ii_ij );
                //luMatrix[ii][ij] -= luMatrix[ii][ik] * luMatrix[ik][ij];
            }
          }

      // U part last block
      for (int ii = lBorder(i), rbi = rBorder(i); ii < rbi; ii++)
        for (int ik = lBorder(i); ik < ii; ik++)
          for (int ij = lBorder(j), rbj = rBorder(j); ij < rbj; ij++)
            luMatrix[ii][ij] -= luMatrix[ii][ik] * luMatrix[ik][ij];
    }

    //L blocks
    for (int j = i + 1; j < bn; j++) {
      __m256d ymm0_ij_ii;
      __m256d ymm1_ij_ik;
      __m256d ymm2_ik_ii;
      for (int k = 0; k < i; k++)
        for (int ij = lBorder(j), rbj = rBorder(j); ij < rbj; ij++)
          for (int ik = lBorder(k), rbk = rBorder(k); ik < rbk; ik++)
          {
            ymm1_ij_ik = _mm256_broadcast_sd( &luMatrix[ij][ik] );
            for (int ii = lBorder(i), rbi = rBorder(i); ii < rbi; ii+=4)
            {
                ymm0_ij_ii = _mm256_loadu_pd( &luMatrix[ij][ii] );
                ymm2_ik_ii = _mm256_loadu_pd( &luMatrix[ik][ii] );
                ymm2_ik_ii = _mm256_mul_pd( ymm2_ik_ii, ymm1_ij_ik );
                ymm0_ij_ii = _mm256_sub_pd( ymm0_ij_ii, ymm2_ik_ii );
                _mm256_storeu_pd(&luMatrix[ij][ii], ymm0_ij_ii );
              //luMatrix[ij][ii] -= luMatrix[ij][ik] * luMatrix[ik][ii];
            }
          }
      // L part last block
      for (int ii = lBorder(i), rbi = rBorder(i); ii < rbi; ii++) {
        for (int ik = lBorder(i); ik < ii; ik++)
          for (int ij = lBorder(j), rbj = rBorder(j); ij < rbj; ij++)
            luMatrix[ij][ii] -= luMatrix[ij][ik] * luMatrix[ik][ii];
        for (int ij = lBorder(j), rbj = rBorder(j); ij < rbj; ij++)
          luMatrix[ij][ii] /= luMatrix[ii][ii];
      }
    }

  }

}


