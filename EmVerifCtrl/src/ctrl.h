#ifndef CTRL_H

#include "typedefine.h"

#define IF_VERSION					( 1u )

#define SAMPLING_KHZ				( 5u )

#define GP_CH_NUM					( 12u )
#define AD_CH_NUM					( 6u )
#define PWM_CH_NUM					( 6u )

#define NOISE_BASE_NUM				( 3u )

#define IN_CH_NUM					( AD_CH_NUM )
#define MIX_OUT_CH_NUM				( PWM_CH_NUM )
#define THROUGH_OUT_CH_NUM			( 12u )

#define CAN_MAX_FIFO_NUM			( 8u )

#define SPI_MAX_DATA_NUM			( 160u )

#define USER_DATA_TO_PC_BYTE_NUM	( 1280u )
#define USER_DATA_FROM_PC_BYTE_NUM	( 1280u )

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

	ui16_t in_val[SAMPLING_KHZ][IN_CH_NUM];						// 60[B]
	ui16_t mix_out_val[SAMPLING_KHZ][MIX_OUT_CH_NUM];			// 60[B]
	ui16_t through_out_val[SAMPLING_KHZ][THROUGH_OUT_CH_NUM];	// 120[B]
} ctrl_user_data_to_pc_st;

typedef union {
	// Warning:
	// 1. Should not add or delete member variables.
	ctrl_user_data_to_pc_st user_struct_data;
	ui8_t user_byte_data[USER_DATA_TO_PC_BYTE_NUM];
} ctrl_user_data_to_pc_ut;

typedef struct {
	// TODO:
	// 1. User setting.
	// Warning:
	// 1. Should not define member variables over max byte size. Max byte size is USER_DATA_FROM_PC_BYTE_NUM.
	ctrl_can_format_st can_send_data[CAN_MAX_FIFO_NUM];									// 128[B]
	ui32_t can_send_num;																// 4[B]

	float sine_hz[THROUGH_OUT_CH_NUM][NOISE_BASE_NUM];									// 144[B]
	float sine_gain[THROUGH_OUT_CH_NUM][NOISE_BASE_NUM];								// 144[B]
	float sine_phase[THROUGH_OUT_CH_NUM][NOISE_BASE_NUM];								// 144[B]
	float white_noise_gain[THROUGH_OUT_CH_NUM];											// 48[B]
	float from_in_to_mix_out_gain[MIX_OUT_CH_NUM][IN_CH_NUM];							// 144[B]
	float from_through_out_to_mix_out_gain[MIX_OUT_CH_NUM][THROUGH_OUT_CH_NUM];			// 288[B]
	ui8_t from_in_to_mix_out_delay_smp[MIX_OUT_CH_NUM][IN_CH_NUM];						// 36[B]
	ui8_t from_through_out_to_mix_out_delay_smp[MIX_OUT_CH_NUM][THROUGH_OUT_CH_NUM];	// 72[B]
	ui16_t square_wave_numerator_cycle;													// 2[B]
	ui16_t square_wave_denominator_cycle;												// 2[B]
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
	ctrl_can_info_st can_info;						// TX: P6_05, RX: P6_04
	ctrl_spi_info_st spi_info;						// CLK: P10_12, MISO: P10_15, MOSI: P10_14, CS: P10_13
	ctrl_square_wave_info_st square_wave_info;		// P04_04
	const bool user_data_from_pc_valid;
	const bool first_flag;
} ctrl_io_data_st;

#endif  /* CTRL_H */
