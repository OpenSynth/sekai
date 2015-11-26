/* decibels neper conversion */

#define LOG10 2.3025850929940459  /*!< natural logarithm of 10 */
#define TWENTY_OVER_LOG10 (20. / LOG10)
#define Np2dB(x) (TWENTY_OVER_LOG10 * (x))
#define dB2Np(x) ((x)/TWENTY_OVER_LOG10)

