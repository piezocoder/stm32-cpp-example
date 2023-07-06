/* UART Embedded Bank - Create accounts, check balance, deposit and withdraw. */
/* Instances of the BankAccount class are stored in a fixed-length vector. */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bank_account.h"
#include <stdio.h>
#include <string.h>
#include "etl/vector.h" /* --- etl library --- */

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
bool UART_ReadChars(uint8_t *buf, uint32_t buf_size, uint32_t delay);
void UART_SendString(const char *msg);
bool get_user_input(const char *prompt, uint8_t *buf, uint32_t buf_size, uint32_t delay);
bool create_account(etl::vector<BankAccount, MAX_ACCOUNTS> &accounts);
bool manage_account(BankAccount &account);
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

  etl::vector<BankAccount, MAX_ACCOUNTS> accounts(MAX_ACCOUNTS);
  const char *prompt = nullptr;
  /* Infinite loop */
  while (1)
  {
    UART_SendString("\r\n*****************************************************\r\n\
------------- Welcome to Embedded Bank! -------------\r\n\
*****************************************************");
    prompt = "\r\nNew account (N) or Existing account (E). \r\nPlease enter: ";
    uint8_t option[OPTIONSIZE] = {0};
    get_user_input(prompt, option, sizeof(option), ENTRY_WAIT); // blocking forever

    if (option[0] == 'N')
    {
      {
        if (!create_account(accounts))
        {
          UART_SendString("\r\nOperation aborted! Please try again!");
          continue;
        }
        const uint16_t new_account_idx = BankAccount::get_total_accounts() - 1;
        if (!manage_account(accounts[new_account_idx]))
        {
          UART_SendString("\r\nOperation aborted! Please try again!");
          continue;
        }
      }
    }
    else if (option[0] == 'E')
    {
      uint8_t account_name[NAMESIZE] = {0};
      prompt = "\r\nEnter account name: ";
      if (!get_user_input(prompt, account_name, sizeof(account_name), TRANSACTION_WAIT))
      {
        UART_SendString("\r\nOperation aborted! Please try again!");
        continue;
      }

      uint8_t password[PASSWORDSIZE] = {0};
      prompt = "\r\nEnter password: ";
      if (!get_user_input(prompt, password, sizeof(password), TRANSACTION_WAIT))
      {
        UART_SendString("\r\nOperation aborted! Please try again!");
        continue;
      }

      // Find the account
      bool account_found = false;
      bool manage_success = false;
      for (auto &account : accounts)
      {
        if (account.verify_account_name(account_name) && account.verify_password(password))
        {
          account_found = true;
          manage_success = manage_account(account);
          break;
        }
      }
      if (!account_found)
        UART_SendString("\r\nInvalid account name or password.");
      else if (!manage_success)
        UART_SendString("\r\nOperation aborted! Please try again!");
    }
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

bool UART_ReadChars(uint8_t *buf, uint32_t buf_size, uint32_t delay)
{
  memset(buf, 0, buf_size);
  uint8_t temp_char = 0;

  for (uint8_t i = 0; i < buf_size - 1; i++)
  {
    if (HAL_UART_Receive(&UartHandle, &temp_char, 1, delay) != HAL_OK)
      return false;
    if (temp_char == '\r')
      break;
    buf[i] = temp_char;
  }
  return true;
}

void UART_SendString(const char *msg)
{
  HAL_UART_Transmit(&UartHandle, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

bool get_user_input(const char *prompt, uint8_t *buf, uint32_t buf_size, uint32_t delay)
{
  UART_SendString(prompt);
  return UART_ReadChars(buf, buf_size, delay);
}

bool create_account(etl::vector<BankAccount, MAX_ACCOUNTS> &accounts)
{
  uint8_t account_name[NAMESIZE] = {0};
  uint8_t password[PASSWORDSIZE] = {0};
  uint8_t confirm_password[PASSWORDSIZE] = {0};
  char tx_buf[100] = {0};
  bool is_unique = true;

  uint16_t count = BankAccount::get_total_accounts();
  if (count == MAX_ACCOUNTS)
    UART_SendString("\r\nThe bank capacity is full. Your account cannot be created.");

  while (true)
  {
    if (!get_user_input("\r\nEnter account name: ", account_name, sizeof(account_name), TRANSACTION_WAIT))
      return false;
    is_unique = true;
    for (auto &account : accounts)
    {
      if (account.verify_account_name(account_name))
      {
        sprintf(tx_buf, "\r\nAccount name '%s' is not available!", account_name);
        UART_SendString(tx_buf);
        is_unique = false;
        break;
      }
    }

    if (is_unique)
      break;
  }

  while (true)
  {
    if (!get_user_input("\r\nEnter password: ", password, sizeof(password), TRANSACTION_WAIT))
      return false;

    if (!get_user_input("\r\nConfirm password: ", confirm_password, sizeof(confirm_password), TRANSACTION_WAIT))
      return false;

    if (memcmp(password, confirm_password, PASSWORDSIZE) != 0)
    {
      UART_SendString("\r\nPassword and confirm password do not match.\n");
      continue;
    }
    BankAccount new_account(account_name, password);
    uint16_t id = new_account.get_account_id();
    accounts[id] = new_account;
    sprintf(tx_buf, "\r\nNew account '%s' created.", account_name);
    UART_SendString(tx_buf);
    return true;
  }
}

bool manage_account(BankAccount &account)
{
  uint8_t option[OPTIONSIZE] = {0};
  uint8_t amount_buf[AMOUNTSIZE] = {0};
  const char *prompt = nullptr;
  double amount = 0;
  char msg[50] = {0};
  while (true)
  {
    prompt = "\r\nBalance (B), Deposit (D), Withdraw (W) or Quit (Q). \r\nPlease enter: ";
    if (!get_user_input(prompt, option, sizeof(option), TRANSACTION_WAIT))
      return false;

    if (option[0] == 'B')
    {
      double balance = account.get_account_balance();
      sprintf(msg, "\r\nBalance: %0.1f", balance);
      UART_SendString(msg);
    }
    else if (option[0] == 'D')
    {
      prompt = "\r\nEnter deposit amount: ";
      if (!get_user_input(prompt, amount_buf, sizeof(amount_buf), TRANSACTION_WAIT))
        return false;
      amount = atof((char *)amount_buf);
      account.deposit(amount);
      sprintf(msg, "\r\nDeposit of %0.1f successful.", amount);
      UART_SendString(msg);
    }
    else if (option[0] == 'W')
    {
      prompt = "\r\nEnter withdrawal amount: ";
      if (!get_user_input(prompt, amount_buf, sizeof(amount_buf), TRANSACTION_WAIT))
        return false;
      amount = atof((char *)amount_buf);
      if (account.withdraw(amount))
      {
        sprintf(msg, "\r\nWithdrawal of %0.1f successful.", amount);
        UART_SendString(msg);
      }
      else
        UART_SendString("\r\nInsufficient balance for withdrawal.");
    }
    else if (option[0] == 'Q')
      break;
    else if (option[0] == 0) // no input, timeout
      continue;
    else
      UART_SendString("\r\nInvalid option.");
  }
  return true;
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
void assert_aborted(uint8_t *file, uint32_t line)
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
