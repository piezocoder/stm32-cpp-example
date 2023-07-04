/* UART Embedded Bank - Create accounts, check balance, deposit and withdraw. */
/* Instances of the BankAccount class are stored in a fixed-size vector. */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bank_account.h"
#include <stdio.h>
#include <string.h>

/* --- etl library --- */
#include "etl/vector.h"
// #include "etl/numeric.h"
// #include <iterator>
// #include <algorithm>

/** @addtogroup STM32F4xx_HAL_Examples
 * @{
 */

/** @addtogroup UART_Hyperterminal_IT
 * @{
 */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MAX_ACCOUNTS 10
#define INACTIVE_DELAY HAL_MAX_DELAY
#define ACTIVE_DELAY 20000
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t rx_buf[NAME_PASSWORD_SIZE] = {0};
uint8_t option = 0;
etl::vector<BankAccount, MAX_ACCOUNTS> accounts(MAX_ACCOUNTS);
/* UART handler declaration */
UART_HandleTypeDef UartHandle;
static GPIO_InitTypeDef GPIO_InitStruct;

/* Interrupt flags */
volatile uint8_t rx_done = 0;
volatile uint8_t tx_done = 0;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Blink(void);
static void UART_Init(void);
static void GPIO_Init(void);
void UART_ReadBytes(uint8_t *buf, uint8_t max_bytes, uint32_t delay);
void UART_SendString(const char *msg);
BankAccount create_account(etl::vector<BankAccount, MAX_ACCOUNTS> &accounts);
void manage_account(BankAccount &account);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  GPIO_Init();
  UART_Init();
  /* Infinite loop */
  while (1)
  {
    UART_SendString("\r\n*****************************************************\r\n\
------------- Welcome to Embedded Bank! -------------\r\n\
*****************************************************");
    UART_SendString("\r\nNew account (N) or Existing account (E). \r\nPlease enter: ");
    UART_ReadBytes(rx_buf, 2, INACTIVE_DELAY);
    option = rx_buf[0];

    if (option == 'N')
    {
      uint16_t count = BankAccount::get_total_accounts();
      if (count == MAX_ACCOUNTS)
        UART_SendString("\r\nThe bank capacity is full. Your account cannot be created.");
      else
      {
        BankAccount new_account = create_account(accounts);
        char temp_buf[100] = {0};
        const uint8_t *new_account_name = new_account.get_account_name();
        sprintf(temp_buf, "\r\nNew account '%s' created.", new_account_name);
        UART_SendString(temp_buf);
        manage_account(new_account);
        accounts[count] = new_account;
      }
    }
    else if (option == 'E')
    {
      uint8_t account_name[NAME_PASSWORD_SIZE] = {0};
      UART_SendString("\r\nEnter account name: ");
      UART_ReadBytes(rx_buf, NAME_PASSWORD_SIZE - 1, ACTIVE_DELAY);
      memcpy(account_name, rx_buf, NAME_PASSWORD_SIZE);

      uint8_t password[NAME_PASSWORD_SIZE] = {0};
      UART_SendString("\r\nEnter password: ");
      UART_ReadBytes(rx_buf, NAME_PASSWORD_SIZE - 1, ACTIVE_DELAY);
      memcpy(password, rx_buf, NAME_PASSWORD_SIZE);

      // Find the account
      bool account_found = false;
      for (auto &account : accounts)
      {
        if (account.verify_account_name(account_name) && account.verify_password(password))
        {
          account_found = true;
          manage_account(account);
          break;
        }
      }
      if (!account_found)
        UART_SendString("\r\nInvalid account name or password.");
    }
    else if (option == 0)
      continue;
    else
      UART_SendString("\r\nInvalid option.");
  }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.  100MHz
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief  Tx Transfer completed callback
 * @param  UartHandle: UART handle.
 * @note   This example shows a simple way to report end of IT Tx transfer, and
 *         you can add your own implementation.
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  tx_done = 1;
}

/**
 * @brief  Rx Transfer completed callback
 * @param  UartHandle: UART handle
 * @note   This example shows a simple way to report end of IT Rx transfer, and
 *         you can add your own implementation.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  rx_done = 1;
}

static void Error_Blink(void)
{
  for (uint8_t i = 0; i < 3; i++)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(500);
  }
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

/**
 * @brief  UART error callbacks
 * @param  UartHandle: UART handle
 * @note   This example shows a simple way to report transfer error, and you can
 *         add your own implementation.
 * @retval None
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  /* Turn LED3 on: Transfer error in reception/transmission process */
  Error_Blink();
}

void UART_Init(void)
{
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /* UART1 configured as follow:
      - Word Length = 8 Bits
      - Stop Bit = One Stop bit
      - Parity = ODD parity
      - BaudRate = 9600 baud
      - Hardware flow control disabled (RTS and CTS signals) */
  UartHandle.Instance = USARTx;
  UartHandle.Init.BaudRate = 9600;
  UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits = UART_STOPBITS_1;
  UartHandle.Init.Parity = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&UartHandle) != HAL_OK)
  {
    Error_Blink();
    while (1)
    {
    }
  }
}

static void GPIO_Init(void)
{
  /*##-1- Enable GPIOC Clock (to be able to program the configuration registers) */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*##-2- Configure PC13 IO in output push-pull mode to drive external LED ###*/
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void UART_ReadBytes(uint8_t *buf, uint8_t max_bytes, uint32_t delay)
{
  memset(buf, 0, NAME_PASSWORD_SIZE);
  uint8_t temp_char = 0;
  for (uint8_t i = 0; i < max_bytes; i++)
  {
    HAL_UART_Receive(&UartHandle, &temp_char, 1, delay);
    if (temp_char == '\r')
      break;
    buf[i] = temp_char;
  }
}

void UART_SendString(const char *msg)
{
  HAL_UART_Transmit(&UartHandle, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

BankAccount create_account(etl::vector<BankAccount, MAX_ACCOUNTS> &accounts)
{
  uint8_t buf[NAME_PASSWORD_SIZE] = {0};
  uint8_t account_name[NAME_PASSWORD_SIZE] = {0};
  uint8_t password[NAME_PASSWORD_SIZE] = {0};
  uint8_t confirm_password[NAME_PASSWORD_SIZE] = {0};
  char tx_buff[100] = {0};
  bool is_unique = true;
  while (true)
  {
    UART_SendString("\r\nEnter account name: ");
    UART_ReadBytes(buf, NAME_PASSWORD_SIZE - 1, ACTIVE_DELAY);
    memcpy(account_name, buf, NAME_PASSWORD_SIZE);
    is_unique = true;
    for (auto &account : accounts)
    {
      if (account.verify_account_name(account_name))
      {
        sprintf(tx_buff, "\r\nAccount name '%s' is not available!", account_name);
        UART_SendString(tx_buff);
        is_unique = false;
        break;
      }
    }

    if (is_unique)
      break;
  }

  while (true)
  {
    UART_SendString("\r\nEnter password: ");
    UART_ReadBytes(buf, NAME_PASSWORD_SIZE - 1, ACTIVE_DELAY);
    memcpy(password, buf, NAME_PASSWORD_SIZE);

    UART_SendString("\r\nConfirm password: ");
    UART_ReadBytes(buf, NAME_PASSWORD_SIZE - 1, ACTIVE_DELAY);
    memcpy(confirm_password, buf, NAME_PASSWORD_SIZE);

    if (memcmp(password, confirm_password, NAME_PASSWORD_SIZE) != 0)
    {
      UART_SendString("\r\nPassword and confirm password do not match.\n");
      continue;
    }
    return BankAccount(account_name, password);
  }
}

void manage_account(BankAccount &account)
{
  uint8_t buf[10] = {0};
  uint8_t choice = 0;
  while (true)
  {
    UART_SendString("\r\nBalance (B), Deposit (D), Withdraw (W) or Quit (Q). \r\nPlease enter: ");
    UART_ReadBytes(buf, 2, ACTIVE_DELAY);
    choice = buf[0];

    if (choice == 'B')
    {
      double balance = account.get_account_balance();
      char msg[20] = {0};
      sprintf(msg, "\r\nBalance: %0.1f", balance);
      UART_SendString(msg);
    }
    else if (choice == 'D')
    {
      UART_SendString("\r\nEnter deposit amount: ");
      UART_ReadBytes(buf, 10, ACTIVE_DELAY);
      double amount = atof((char *)buf);
      account.deposit(amount);
      UART_SendString("\r\nDeposit successful.");
    }
    else if (choice == 'W')
    {
      UART_SendString("\r\nEnter withdrawal amount: ");
      UART_ReadBytes(buf, 10, ACTIVE_DELAY);
      double amount = atof((char *)buf);
      if (account.withdraw(amount))
        UART_SendString("\r\nWithdrawal successful.");
      else
        UART_SendString("\r\nInsufficient balance for withdrawal.");
    }
    else if (choice == 'Q')
      break;
    else
      UART_SendString("\r\nInvalid option.");
  }
}

static void Error_Handler(void)
{
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
 * @}
 */

/**
 * @}
 */
