/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum StartButtonRole
{
  RESTART,
  BEGIN,
  NONE
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SEGMENTS 7

void updateLEDs();
double getWeight(int);
int* digitToHexDisplay(int);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Variables to help button logics
enum StartButtonRole startRole = BEGIN;
bool isButtonFree = true;
bool isSettingUp = true;

// Variables to help game logics
uint32_t adcValue = 0;
int holeScores[9] = { 50, 150, 75, 300, 500, 200, 25, 100, 25 };
double weightPerBag = 0.0;
int currentTeam = 1;
int team1Score = 0;
int team1TargetScore = 0;
int team2Score = 0;
int team2TargetScore = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc);
  HAL_ADC_Start_DMA(&hadc, &adcValue, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Update team score if the game is running
    if (startRole == NONE)
    {
      // Calculate the total score from all holes
      int totalScore;
      for (int i = 0; i < 9; i++)
      {
        totalScore += round(getWeight(i) / weightPerBag) * holeScores[i];
      }

      if (currentTeam == 1)
      {
        // Game ends
        if (totalScore == team1TargetScore)
        {
          startRole = RESTART;
        }
        else if (totalScore < team1TargetScore)
        {
          team1Score = team1TargetScore - totalScore;
        }
        else
        {
          // TODO Blink score
        }
      }
      else
      {
        // Game ends
        if (totalScore == team2TargetScore)
        {
          startRole = RESTART;
        }
        else if (totalScore < team2TargetScore)
        {
          team2Score = team2TargetScore - totalScore;
        }
        else
        {
          // TODO Blink score
        }
      }
    }

    updateLEDs();
    // TODO Update the HEX display
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// Interrupt handler
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // Most of the times, buttons are just pressed
  if (isButtonFree)
  {
    HAL_TIM_Base_Start_IT(&htim1);
    isButtonFree = false;
  }

  // These can trigger the "buttons held" logics
  if (GPIO_Pin == START_RESET_BUTTON_Pin ||
      (HAL_GPIO_ReadPin(REMOVE_SCORE_BUTTON_GPIO_Port, REMOVE_SCORE_BUTTON_Pin) == GPIO_PIN_RESET &&
       HAL_GPIO_ReadPin(ADD_SCORE_BUTTON_GPIO_Port, ADD_SCORE_BUTTON_Pin) == GPIO_PIN_RESET))
  {
    // Restart the timer
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_Base_Start_IT(&htim3);
  }
}

void updateLEDs()
{
  HAL_GPIO_WritePin(TEAM1_LED_EN_GPIO_Port, TEAM1_LED_EN_Pin, (GPIO_PinState) (currentTeam == 1));
  HAL_GPIO_WritePin(TEAM2_LED_EN_GPIO_Port, TEAM2_LED_EN_Pin, (GPIO_PinState) (currentTeam == 2));
}

// Reset the game
void restartGame()
{
  startRole = BEGIN;
  isButtonFree = true;
  isSettingUp = true;
  weightPerBag = 0;
  currentTeam = 1;
  team1Score = 0;
  team1TargetScore = 0;
  team2Score = 0;
  team2TargetScore = 0;

  // TODO Reset everything (the score board, target score, ...)
  updateLEDs();

  // Stop the timers
  HAL_TIM_Base_Stop_IT(&htim1);
  HAL_TIM_Base_Stop_IT(&htim3);
  HAL_TIM_Base_Stop_IT(&htim14);
}

double getWeight(int fsrNum)
{
  // Set the MUX to get value from the interested FSR
  HAL_GPIO_WritePin(AIN_S0_GPIO_Port, AIN_S0_Pin, (GPIO_PinState) ((fsrNum >> 0) & 1));
  HAL_GPIO_WritePin(AIN_S1_GPIO_Port, AIN_S1_Pin, (GPIO_PinState) ((fsrNum >> 1) & 1));
  HAL_GPIO_WritePin(AIN_S2_GPIO_Port, AIN_S2_Pin, (GPIO_PinState) ((fsrNum >> 2) & 1));
  HAL_GPIO_WritePin(AIN_S3_GPIO_Port, AIN_S3_Pin, (GPIO_PinState) ((fsrNum >> 3) & 1));

  // TODO Wait for the ADC to balance itself
  return exp((adcValue - 1384.04) / 307.17) + 19.98;
}

void beginGame()
{
  if (team1TargetScore > 0 && team2TargetScore > 0 && weightPerBag > 0)
  {
    startRole = NONE;
    isSettingUp = false;
    team1Score = team1TargetScore;
    team2Score = team2TargetScore;
  }
  else
  {
    // TODO Display an error message. Do we want different messages for different errors?
  }
}

// De-bouncing timer handler
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // Case 1: Buttons are pressed
  if (htim == &htim1)
  {
    if (HAL_GPIO_ReadPin(START_RESET_BUTTON_GPIO_Port, START_RESET_BUTTON_Pin) == GPIO_PIN_SET)
    {
      switch (startRole)
      {
        case RESTART:
          restartGame();
          break;
        case BEGIN:
          beginGame();
          break;
        default:
          break;
      }
    }
    // Switch team
    else if (HAL_GPIO_ReadPin(SELECT_TEAM_BUTTON_GPIO_Port, SELECT_TEAM_BUTTON_Pin) == GPIO_PIN_SET)
    {
      currentTeam = (currentTeam == 1) ? 2 : 1;
    }
    // Set target score
    else if (isSettingUp)
    {
      // Increase target score
      if (HAL_GPIO_ReadPin(ADD_SCORE_BUTTON_GPIO_Port, ADD_SCORE_BUTTON_Pin) == GPIO_PIN_SET)
      {
        if (currentTeam == 1) { team1TargetScore += 100; }
        else { team2TargetScore += 100; }
      }
      // Decrease target score
      else if (HAL_GPIO_ReadPin(REMOVE_SCORE_BUTTON_GPIO_Port, REMOVE_SCORE_BUTTON_Pin) == GPIO_PIN_SET)
      {
        if (currentTeam == 1) { if (team1TargetScore > 0) { team1TargetScore -= 100; } }
        else if (team2TargetScore > 0) { team2TargetScore -= 100; }
      }
    }
    // Manually adjust score
    else
    {
      // Increase score
      if (HAL_GPIO_ReadPin(ADD_SCORE_BUTTON_GPIO_Port, ADD_SCORE_BUTTON_Pin) == GPIO_PIN_SET)
      {
        if (currentTeam == 1) { team1Score += 25; }
        else { team2Score += 25; }
      }
      // Decrease score
      else if (HAL_GPIO_ReadPin(REMOVE_SCORE_BUTTON_GPIO_Port, REMOVE_SCORE_BUTTON_Pin) == GPIO_PIN_SET)
      {
        if (currentTeam == 1) { if (team1Score > 0) { team1Score -= 25; } }
        else if (team2Score > 0) { team2Score -= 25; }
      }
    }

    isButtonFree = true;
    HAL_TIM_Base_Stop_IT(&htim1);
  }
  // Case 2: Buttons are held
  else if (htim == &htim3)
  {
    // Reset
    if (HAL_GPIO_ReadPin(START_RESET_BUTTON_GPIO_Port, START_RESET_BUTTON_Pin) == GPIO_PIN_RESET)
    {
      restartGame();
    }
    // Calibrate
    else if (isSettingUp && HAL_GPIO_ReadPin(ADD_SCORE_BUTTON_GPIO_Port, ADD_SCORE_BUTTON_Pin) == GPIO_PIN_RESET &&
    	HAL_GPIO_ReadPin(REMOVE_SCORE_BUTTON_GPIO_Port, REMOVE_SCORE_BUTTON_Pin) == GPIO_PIN_RESET)
    {
    	weightPerBag = getWeight(0);
    }

    HAL_TIM_Base_Stop_IT(&htim3);
  }
  // Case 3: Flash the score board
  else if (htim == &htim14)
  {
    // TODO Flash the score
  }
}

void displayTeam1Score() {
	int dig1 = team1Score / 1000;
	int dig2 = (team1Score / 100) % 10;
	int dig3 = (team1Score / 10) % 10;
	int dig4 = team1Score % 10;

	int* dig1Segments = digitToHexDisplay(dig1);
  // TODO - Fill the shift register
	for (int i=6;i>=0;i--) {
    HAL_GPIO_WritePin(TEAM1_SWD_GPIO_Port, TEAM1_SWD_Pin, (GPIO_PinState) dig1Segments[i]);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_RESET);
  }
  HAL_GPIO_WritePin(TEAM1_LATCH_OUTPUT_GPIO_Port, TEAM1_LATCH_OUTPUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TEAM1_LATCH_OUTPUT_GPIO_Port, TEAM1_LATCH_OUTPUT_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TEAM1_DIGIT1_EN_GPIO_Port,TEAM1_DIGIT1_EN_Pin, GPIO_PIN_SET); //Enable Dig1 
  HAL_GPIO_WritePin(TEAM1_DIGIT1_EN_GPIO_Port,TEAM1_DIGIT1_EN_Pin, GPIO_PIN_RESET); //Disable Dig 1
  free(dig1Segments);

	int* dig2Segments = digitToHexDisplay(dig2);
    // TODO - Fill the shift register
	for (int i=6;i>=0;i--) {
    HAL_GPIO_WritePin(TEAM1_SWD_GPIO_Port, TEAM1_SWD_Pin, (GPIO_PinState) dig2Segments[i]);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_RESET);
  }
  HAL_GPIO_WritePin(TEAM1_LATCH_OUTPUT_GPIO_Port, TEAM1_LATCH_OUTPUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TEAM1_DIGIT2_EN_GPIO_Port,TEAM1_DIGIT2_EN_Pin, GPIO_PIN_SET); //Enable Dig2
  HAL_GPIO_WritePin(TEAM1_DIGIT2_EN_GPIO_Port,TEAM1_DIGIT2_EN_Pin, GPIO_PIN_RESET); //Disable Dig2
  free(dig2Segments);

	int* dig3Segments = digitToHexDisplay(dig3);
    // TODO - Fill the shift register
	for (int i=6;i>=0;i--) {
    HAL_GPIO_WritePin(TEAM1_SWD_GPIO_Port, TEAM1_SWD_Pin, (GPIO_PinState) dig3Segments[i]);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_RESET);
  }
  HAL_GPIO_WritePin(TEAM1_LATCH_OUTPUT_GPIO_Port, TEAM1_LATCH_OUTPUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TEAM1_DIGIT3_EN_GPIO_Port,TEAM1_DIGIT3_EN_Pin, GPIO_PIN_SET); //Enable Dig3
  HAL_GPIO_WritePin(TEAM1_DIGIT3_EN_GPIO_Port,TEAM1_DIGIT3_EN_Pin, GPIO_PIN_RESET); //Disable Dig3
  free(dig3Segments);

	int* dig4Segments = digitToHexDisplay(dig4);
    // TODO - Fill the shift register
	for (int i=6;i>=0;i--) {
    HAL_GPIO_WritePin(TEAM1_SWD_GPIO_Port, TEAM1_SWD_Pin, (GPIO_PinState) dig4Segments[i]);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TEAM1_SWCLK_GPIO_Port, TEAM1_SWCLK_Pin, GPIO_PIN_RESET);
  }
  HAL_GPIO_WritePin(TEAM1_LATCH_OUTPUT_GPIO_Port, TEAM1_LATCH_OUTPUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TEAM1_DIGIT4_EN_GPIO_Port,TEAM1_DIGIT4_EN_Pin, GPIO_PIN_SET); //Enable Dig4
  HAL_GPIO_WritePin(TEAM1_DIGIT4_EN_GPIO_Port,TEAM1_DIGIT4_EN_Pin, GPIO_PIN_RESET); //Disable Dig4
  free(dig4Segments);
}

int* digitToHexDisplay(int digit) {
	int *segments = (int *)malloc(SEGMENTS * sizeof(int));
    switch (digit) {
        case 0:
            segments[0] = 1; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 1; // d
            segments[4] = 1; // e
            segments[5] = 1; // f
            segments[6] = 0; // g
            break;
        case 1:
            segments[0] = 0; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 0; // d
            segments[4] = 0; // e
            segments[5] = 0; // f
            segments[6] = 0; // g
            break;
        case 2:
            segments[0] = 1; // a
            segments[1] = 1; // b
            segments[2] = 0; // c
            segments[3] = 1; // d
            segments[4] = 1; // e
            segments[5] = 0; // f
            segments[6] = 1; // g
            break;
        case 3:
            segments[0] = 1; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 1; // d
            segments[4] = 0; // e
            segments[5] = 0; // f
            segments[6] = 1; // g
            break;
        case 4:
            segments[0] = 0; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 0; // d
            segments[4] = 0; // e
            segments[5] = 1; // f
            segments[6] = 1; // g
            break;
        case 5:
            segments[0] = 1; // a
            segments[1] = 0; // b
            segments[2] = 1; // c
            segments[3] = 1; // d
            segments[4] = 0; // e
            segments[5] = 1; // f
            segments[6] = 1; // g
            break;
        case 6:
            segments[0] = 1; // a
            segments[1] = 0; // b
            segments[2] = 1; // c
            segments[3] = 1; // d
            segments[4] = 1; // e
            segments[5] = 1; // f
            segments[6] = 1; // g
            break;
        case 7:
            segments[0] = 1; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 0; // d
            segments[4] = 0; // e
            segments[5] = 0; // f
            segments[6] = 0; // g
            break;
        case 8:
            segments[0] = 1; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 1; // d
            segments[4] = 1; // e
            segments[5] = 1; // f
            segments[6] = 1; // g
            break;
        case 9:
            segments[0] = 1; // a
            segments[1] = 1; // b
            segments[2] = 1; // c
            segments[3] = 1; // d
            segments[4] = 0; // e
            segments[5] = 1; // f
            segments[6] = 1; // g
            break;
        default:
            // If digit is out of range, turn off all segments
            for (int i = 0; i < SEGMENTS; i++) {
                segments[i] = 0;
            }
            break;
    }
	return segments;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
