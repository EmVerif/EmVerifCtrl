#ifndef CTRL_H

#include "typedefine.h"

#define IF_VERSION					( 2u )

#define SAMPLING_KHZ				( 10u )

#define GP_CH_NUM					( 12u )
#define AD_CH_NUM					( 6u )
#define PWM_CH_NUM					( 6u )

#define SPI_MAX_DATA_NUM			( 160u )

#define SPIOUT_CH_NUM			    ( 12u )
#define SINE_BASE_NUM				( 3u )

#define CAN_MAX_FIFO_NUM			( 8u )

#define USER_DATA_TO_PC_BYTE_NUM	( 1280u )
#define USER_DATA_FROM_PC_BYTE_NUM	( 1280u )

#define MAX_DELAY_SMP				( 256u + SAMPLING_KHZ )

typedef struct {
	// Warning:
	// 1. Should not add or delete member variables.
	// 2. Should not reorder member variables.
	ui32_t canid;
	bool is_extend_id;
	bool is_remote_frame;
	ui8_t data_len;
	ui8_t data[8];
} ctrl_can_format_st;

typedef struct {
	// Warning:
	// 1. Should not add or delete member variables.
	// 2. Should not reorder member variables.
	// 3. Should not rewrite constant data.
	const bool enable;
	const ctrl_can_format_st can_recv_data[CAN_MAX_FIFO_NUM];
	const ctrl_can_format_st can_send_fin_data[CAN_MAX_FIFO_NUM];
	ctrl_can_format_st can_send_data[CAN_MAX_FIFO_NUM];
	const ui32_t recv_num;
	const ui32_t send_fin_num;
	const ui32_t send_possible_num;
	ui32_t send_request_num;
	const ui32_t cerror;
	const ui32_t cstatus;
	const bool recv_overrun_flag;
} ctrl_can_info_st;

typedef struct {
	// Warning:
	// 1. Should not add or delete member variables.
	// 2. Should not reorder member variables.
	// 3. Should not rewrite constant data.
	const bool enable;
	ui32_t send_num[SAMPLING_KHZ];
	ui32_t send_buf[SPI_MAX_DATA_NUM];
	bool cs_keep_flag[SPI_MAX_DATA_NUM];
	const ui32_t recv_size[SAMPLING_KHZ];
	const ui32_t recv_buf[SPI_MAX_DATA_NUM];
} ctrl_spi_info_st;

typedef struct {
	// Warning:
	// 1. Should not add or delete member variables.
	// 2. Should not reorder member variables.
	// 3. Should not rewrite constant data.
	// 1cycle = 33.333333 / 128[MHz]
	ui16_t numerator_cycle;
	ui16_t denominator_cycle;
} ctrl_square_wave_info_st;

typedef struct {
	// TODO:
	// 1. User setting.
	// Warning:
	// 1. Should not define member variables over max byte size. Max byte size is USER_DATA_TO_PC_BYTE_NUM.
	ctrl_can_format_st can_recv_data[CAN_MAX_FIFO_NUM];			// 128[B]
	ctrl_can_format_st can_send_fin_data[CAN_MAX_FIFO_NUM];		// 128[B]
	ui32_t can_recv_num;										// 4[B]
	ui32_t can_send_fin_num;									// 4[B]
	ui32_t can_send_possible_num;								// 4[B]
	ui32_t cerror;												// 4[B]
	ui32_t cstatus;												// 4[B]

	ui32_t timestamp;											// 4[B]

	ui16_t ad_val[SAMPLING_KHZ][AD_CH_NUM];						// 120[B]
	ui16_t pwm_val[SAMPLING_KHZ][PWM_CH_NUM];					// 120[B]
	ui16_t spiout_val[SAMPLING_KHZ][SPIOUT_CH_NUM];				// 240[B]
} ctrl_user_data_to_pc_st;

typedef union {
	// Warning:
	// 1. Should not add or delete member variables.
	ctrl_user_data_to_pc_st user_struct_data;
	ui8_t user_byte_data[USER_DATA_TO_PC_BYTE_NUM];
} ctrl_user_data_to_pc_ut;

typedef enum {
	CTRL_USER_DATA_FROM_PC_0,
	CTRL_USER_DATA_FROM_PC_1
} ctrl_user_data_from_pc_type_et;

typedef struct {
	ctrl_can_format_st can_send_data[CAN_MAX_FIFO_NUM];									// 128[B]
	ui32_t can_send_num;																// 4[B]

	float pwm_sine_hz[PWM_CH_NUM][SINE_BASE_NUM];										// 72[B]
	float pwm_sine_gain[PWM_CH_NUM][SINE_BASE_NUM];										// 72[B]
	float pwm_sine_phase[PWM_CH_NUM][SINE_BASE_NUM];									// 72[B]
	float pwm_white_noise_gain[PWM_CH_NUM];												// 24[B]

	float spiout_sine_hz[SPIOUT_CH_NUM][SINE_BASE_NUM];									// 144[B]
	float spiout_sine_gain[SPIOUT_CH_NUM][SINE_BASE_NUM];								// 144[B]
	float spiout_sine_phase[SPIOUT_CH_NUM][SINE_BASE_NUM];								// 144[B]
	float spiout_white_noise_gain[SPIOUT_CH_NUM];										// 48[B]

	ui16_t square_wave_numerator_cycle;													// 2[B]
	ui16_t square_wave_denominator_cycle;												// 2[B]

	ui8_t port_out_val;																	// 1[B]
} ctrl_user_data_from_pc_0_st;

typedef struct {
	float from_ad_to_pwm_gain[PWM_CH_NUM][AD_CH_NUM];									// 144[B]
	float from_spiout_to_pwm_gain[PWM_CH_NUM][SPIOUT_CH_NUM];							// 288[B]
	float from_ad_to_spiout_gain[SPIOUT_CH_NUM][AD_CH_NUM];								// 288[B]

	ui8_t from_ad_to_pwm_delay_smp[PWM_CH_NUM][AD_CH_NUM];								// 36[B]
	ui8_t from_spiout_to_pwm_delay_smp[PWM_CH_NUM][SPIOUT_CH_NUM];						// 72[B]
	ui8_t from_ad_to_spiout_delay_smp[SPIOUT_CH_NUM][AD_CH_NUM];						// 72[B]

	ui8_t dc_cut_flag;																	// 1[B]
} ctrl_user_data_from_pc_1_st;

typedef union {
	ctrl_user_data_from_pc_0_st user_data_0;											// 857[B]
	ctrl_user_data_from_pc_1_st user_data_1;											// 901[B]
} ctrl_user_data_from_pc_type_ut;

typedef struct {
	// TODO:
	// 1. User setting.
	// Warning:
	// 1. Should not define member variables over max byte size. Max byte size is USER_DATA_FROM_PC_BYTE_NUM.
	ctrl_user_data_from_pc_type_et type;												// 4[B]
	ctrl_user_data_from_pc_type_ut user_union_data;										// 900[B]
} ctrl_user_data_from_pc_st;

typedef union {
	// Warning:
	// 1. Should not add or delete member variables.
	ctrl_user_data_from_pc_st user_struct_data;
	ui8_t user_byte_data[USER_DATA_FROM_PC_BYTE_NUM];
} ctrl_user_data_from_pc_ut;

typedef struct {
	// Warning:
	// 1. Should not add or delete member variables.
	// 2. Should not reorder member variables.
	// 3. Should not rewrite const data.
	ctrl_user_data_to_pc_ut user_data_to_pc;
	const ctrl_user_data_from_pc_ut user_data_from_pc;
	const ui16_t ad_val[SAMPLING_KHZ][AD_CH_NUM];	// P01_08, P01_09, P01_10, P01_11, P01_12, P01_13
	ui16_t pwm_val[SAMPLING_KHZ][PWM_CH_NUM];		// P05_00, P05_01, P05_05, P05_07, P08_14, P08_15
	ctrl_can_info_st can_info;						// TX: P06_05, RX: P06_04
	ctrl_spi_info_st spi_info;						// CLK: P10_12, MISO: P10_15, MOSI: P10_14, CS: P10_13
	ctrl_square_wave_info_st square_wave_info;		// P04_04
	ui8_t gpio_val;									// P03_08, P03_09, P03_10, P03_11, P03_12, P03_13, P03_14, P03_15
	const bool user_data_from_pc_valid;
	const bool first_flag;
} ctrl_io_data_st;

#endif  /* CTRL_H */
