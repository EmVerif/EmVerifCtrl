#include <math.h>
#include <string.h>
#include "typedefine.h"
#include "ctrl.h"

// TODO: Sample program
static ctrl_user_data_from_pc_0_st ctrl_user_data_from_pc_0;
static ctrl_user_data_from_pc_1_st ctrl_user_data_from_pc_1;

static float ctrl_pwm_sine_current_phase[PWM_CH_NUM][SINE_BASE_NUM];
static float ctrl_spiout_sine_current_phase[SPIOUT_CH_NUM][SINE_BASE_NUM];
static ui32_t ctrl_timestamp;

static const float ctrl_pi = 3.14159265358979323846;
static const float ctrl_phase_step_per_hz = ( 2.0 * ctrl_pi ) / SAMPLING_KHZ / 1000;

static float ctrl_ad_delay_buf[MAX_DELAY_SMP][AD_CH_NUM];
static float ctrl_spiout_delay_buf[MAX_DELAY_SMP][SPIOUT_CH_NUM];
static ui16_t ctrl_delay_buf_wpt;

static ui16_t ctrl_pwm_random[SAMPLING_KHZ][PWM_CH_NUM];
static ui16_t ctrl_spiout_random[SAMPLING_KHZ][SPIOUT_CH_NUM];
static ui16_t ctrl_random_x;
static ui16_t ctrl_random_c;

static float ctrl_fw_delay_buf[AD_CH_NUM];
static float ctrl_fb_delay_buf[AD_CH_NUM];

static void ctrl_init( void )
{
	memset( (void*)&ctrl_user_data_from_pc_0, 0, sizeof(ctrl_user_data_from_pc_0) );
	memset( (void*)&ctrl_user_data_from_pc_1, 0, sizeof(ctrl_user_data_from_pc_1) );

	for( ui32_t idx = 0; idx < SINE_BASE_NUM; idx++ )
	{
		for( ui32_t ch = 0; ch < PWM_CH_NUM; ch++ )
		{
			ctrl_pwm_sine_current_phase[ch][idx] = 0.0;
		}
		for( ui32_t ch = 0; ch < SPIOUT_CH_NUM; ch++ )
		{
			ctrl_spiout_sine_current_phase[ch][idx] = 0.0;
		}
	}
	ctrl_timestamp = 0u;

	memset( (void*)&ctrl_ad_delay_buf[0][0], 0, sizeof(ctrl_ad_delay_buf) );
	memset( (void*)&ctrl_spiout_delay_buf[0][0], 0, sizeof(ctrl_spiout_delay_buf) );
	ctrl_delay_buf_wpt = 0u;

	ctrl_random_x = 1u;
	ctrl_random_c = 1u;

	for( ui32_t ch = 0; ch < AD_CH_NUM; ch++ )
	{
		ctrl_fw_delay_buf[ch] = 0.0;
		ctrl_fb_delay_buf[ch] = 0.0;
	}
}

static void ctrl_copy_param( ctrl_io_data_st* io_data_p )
{
	if( io_data_p->user_data_from_pc_valid )
	{
		switch( io_data_p->user_data_from_pc.user_struct_data.type )
		{
		case CTRL_USER_DATA_FROM_PC_0:
			ctrl_user_data_from_pc_0 = io_data_p->user_data_from_pc.user_struct_data.user_union_data.user_data_0;
			break;
		case CTRL_USER_DATA_FROM_PC_1:
			ctrl_user_data_from_pc_1 = io_data_p->user_data_from_pc.user_struct_data.user_union_data.user_data_1;
			break;
		default:
			break;
		}
	}
}

static void ctrl_generate_random( void )
{
	ui32_t x = ctrl_random_x;
	ui32_t c = ctrl_random_c;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( si32_t ch = 0; ch < PWM_CH_NUM; ch++ )
		{
			ui32_t tmp = ( 31743u * x ) + c;

			x = (ui16_t)( tmp % 65536 );
			c = (ui16_t)( tmp / 65536 );
			ctrl_pwm_random[smp][ch] = x;
		}
		for( si32_t ch = 0; ch < SPIOUT_CH_NUM; ch++ )
		{
			ui32_t tmp = ( 31743u * x ) + c;

			x = (ui16_t)( tmp % 65536 );
			c = (ui16_t)( tmp / 65536 );
			ctrl_spiout_random[smp][ch] = x;
		}
	}
	ctrl_random_x = x;
	ctrl_random_c = c;
}

static void ctrl_generate_pwm_base_signal( float out_pwm_base_signal[SAMPLING_KHZ][PWM_CH_NUM] )
{
	float base_signal;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t ch = 0; ch < PWM_CH_NUM; ch++ )
		{
			base_signal = 0.0;
			for( ui32_t idx = 0; idx < SINE_BASE_NUM; idx++ )
			{
				base_signal += 32768 * sin( ctrl_pwm_sine_current_phase[ch][idx] + ctrl_user_data_from_pc_0.pwm_sine_phase[ch][idx] ) * ctrl_user_data_from_pc_0.pwm_sine_gain[ch][idx];
				ctrl_pwm_sine_current_phase[ch][idx] += ( ctrl_user_data_from_pc_0.pwm_sine_hz[ch][idx] * ctrl_phase_step_per_hz );
				if( ctrl_pwm_sine_current_phase[ch][idx] >= ( 2 * ctrl_pi ) )
				{
					ctrl_pwm_sine_current_phase[ch][idx] -= ( 2 * ctrl_pi );
				}
			}
			base_signal += ( ctrl_pwm_random[smp][ch] - 32768 ) * ctrl_user_data_from_pc_0.pwm_white_noise_gain[ch];
			out_pwm_base_signal[smp][ch] = base_signal;
		}
	}
}

static void ctrl_generate_spiout_base_signal( float out_spiout_base_signal[SAMPLING_KHZ][SPIOUT_CH_NUM] )
{
	float base_signal;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t ch = 0; ch < SPIOUT_CH_NUM; ch++ )
		{
			base_signal = 0.0;
			for( ui32_t idx = 0; idx < SINE_BASE_NUM; idx++ )
			{
				base_signal += 32768 * sin( ctrl_spiout_sine_current_phase[ch][idx] + ctrl_user_data_from_pc_0.spiout_sine_phase[ch][idx] ) * ctrl_user_data_from_pc_0.spiout_sine_gain[ch][idx];
				ctrl_spiout_sine_current_phase[ch][idx] += ( ctrl_user_data_from_pc_0.spiout_sine_hz[ch][idx] * ctrl_phase_step_per_hz );
				if( ctrl_spiout_sine_current_phase[ch][idx] >= ( 2 * ctrl_pi ) )
				{
					ctrl_spiout_sine_current_phase[ch][idx] -= ( 2 * ctrl_pi );
				}
			}
			base_signal += ( ctrl_spiout_random[smp][ch] - 32768 ) * ctrl_user_data_from_pc_0.spiout_white_noise_gain[ch];
			out_spiout_base_signal[smp][ch] = base_signal;
		}
	}
}

static void ctrl_adapt_hpf(
	const ui16_t in_ad_val[SAMPLING_KHZ][AD_CH_NUM],
	float out_ad_val_dccut[SAMPLING_KHZ][AD_CH_NUM],
	ui16_t out_ad_val_dccut_for_send[SAMPLING_KHZ][AD_CH_NUM]
)
{
	if( ctrl_user_data_from_pc_1.dc_cut_flag != 0u )
	{
		for( si32_t ch = 0; ch < AD_CH_NUM; ch++ )
		{
			float in_ad;
			float out_ad;
			float prev_in_ad = ctrl_fw_delay_buf[ch];
			float prev_out_ad = ctrl_fb_delay_buf[ch];

			for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
			{
				ui16_t out_ad_for_send;

				in_ad = ( (si32_t)in_ad_val[smp][ch] - 32768 );
				out_ad = in_ad;
				out_ad -= prev_in_ad;
				out_ad += 0.9999 * prev_out_ad;
				out_ad_val_dccut[smp][ch] = out_ad;
				if( out_ad > 32767 )
				{
					out_ad_for_send = 65535;
				}
				else if( out_ad < -32768 )
				{
					out_ad_for_send = 0;
				}
				else
				{
					out_ad_for_send = (ui16_t)( out_ad + 32768 );
				}
				out_ad_val_dccut_for_send[smp][ch] = out_ad_for_send;
				prev_in_ad = in_ad;
				prev_out_ad = out_ad;
			}
			ctrl_fw_delay_buf[ch] = in_ad;
			ctrl_fb_delay_buf[ch] = out_ad;
		}
	}
	else
	{
		for( si32_t ch = 0; ch < AD_CH_NUM; ch++ )
		{
			for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
			{
				out_ad_val_dccut[smp][ch] = ( (si32_t)in_ad_val[smp][ch] - 32768 );
				out_ad_val_dccut_for_send[smp][ch] = in_ad_val[smp][ch];
			}
			ctrl_fw_delay_buf[ch] = out_ad_val_dccut[SAMPLING_KHZ - 1][ch];
			ctrl_fb_delay_buf[ch] = out_ad_val_dccut[SAMPLING_KHZ - 1][ch];
		}
	}
}

static void ctrl_set_delay_buf(
	const float in_ad_val[SAMPLING_KHZ][AD_CH_NUM],
	const float in_spiout_base_signal[SAMPLING_KHZ][SPIOUT_CH_NUM]
)
{
	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( si32_t ch = 0; ch < AD_CH_NUM; ch++ )
		{
			ctrl_ad_delay_buf[ctrl_delay_buf_wpt][ch] = in_ad_val[smp][ch];
		}
		for( si32_t ch = 0; ch < SPIOUT_CH_NUM; ch++ )
		{
			ctrl_spiout_delay_buf[ctrl_delay_buf_wpt][ch] = in_spiout_base_signal[smp][ch];
		}
		ctrl_delay_buf_wpt++;
		if( ctrl_delay_buf_wpt >= MAX_DELAY_SMP )
		{
			ctrl_delay_buf_wpt = 0u;
		}
	}
}

static void ctrl_get_delay_buf(
	float out_ad_to_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM][AD_CH_NUM],
	float out_ad_to_spiout_signal[SAMPLING_KHZ][SPIOUT_CH_NUM][AD_CH_NUM],
	float out_spiout_to_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM][SPIOUT_CH_NUM]
)
{
	si32_t rd_pt;
	si32_t rd_init_pt = (si32_t)ctrl_delay_buf_wpt - SAMPLING_KHZ;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( si32_t from_ch = 0; from_ch < AD_CH_NUM; from_ch++ )
		{
			for( si32_t to_ch = 0; to_ch < PWM_CH_NUM; to_ch++ )
			{
				rd_pt = rd_init_pt - (si32_t)ctrl_user_data_from_pc_1.from_ad_to_pwm_delay_smp[to_ch][from_ch];
				if( rd_pt < 0 )
				{
					rd_pt += MAX_DELAY_SMP;
				}
				out_ad_to_pwm_signal[smp][to_ch][from_ch] = ctrl_ad_delay_buf[rd_pt][from_ch];
			}
			for( si32_t to_ch = 0; to_ch < SPIOUT_CH_NUM; to_ch++ )
			{
				rd_pt = rd_init_pt - (si32_t)ctrl_user_data_from_pc_1.from_ad_to_spiout_delay_smp[to_ch][from_ch];
				if( rd_pt < 0 )
				{
					rd_pt += MAX_DELAY_SMP;
				}
				out_ad_to_spiout_signal[smp][to_ch][from_ch] = ctrl_ad_delay_buf[rd_pt][from_ch];
			}
		}
		for( si32_t from_ch = 0; from_ch < SPIOUT_CH_NUM; from_ch++ )
		{
			for( si32_t to_ch = 0; to_ch < PWM_CH_NUM; to_ch++ )
			{
				rd_pt = rd_init_pt - (si32_t)ctrl_user_data_from_pc_1.from_spiout_to_pwm_delay_smp[to_ch][from_ch];
				if( rd_pt < 0 )
				{
					rd_pt += MAX_DELAY_SMP;
				}
				out_spiout_to_pwm_signal[smp][to_ch][from_ch] = ctrl_spiout_delay_buf[rd_pt][from_ch];
			}
		}
		rd_init_pt++;
	}
}

static void ctrl_generate_pwm_signal(
	const float in_ad_to_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM][AD_CH_NUM],
	const float in_spiout_to_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM][SPIOUT_CH_NUM],
	const float in_pwm_base_signal[SAMPLING_KHZ][PWM_CH_NUM],
	ui16_t out_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM]
)
{
	float signal;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t to_ch = 0; to_ch < PWM_CH_NUM; to_ch++ )
		{
			signal = in_pwm_base_signal[smp][to_ch];
			for( ui32_t from_ch = 0; from_ch < AD_CH_NUM; from_ch++ )
			{
				signal += in_ad_to_pwm_signal[smp][to_ch][from_ch] * ctrl_user_data_from_pc_1.from_ad_to_pwm_gain[to_ch][from_ch];
			}
			for( ui32_t from_ch = 0; from_ch < SPIOUT_CH_NUM; from_ch++ )
			{
				signal += in_spiout_to_pwm_signal[smp][to_ch][from_ch] * ctrl_user_data_from_pc_1.from_spiout_to_pwm_gain[to_ch][from_ch];
			}
			if( signal > 32767 )
			{
				signal = 65535;
			}
			else if( signal < -32768 )
			{
				signal = 0;
			}
			else
			{
				signal += 32768;
			}
			out_pwm_signal[smp][to_ch] = (ui16_t)signal;
		}
	}
}

static void ctrl_generate_spiout_val(
	const float in_ad_to_spiout_signal[SAMPLING_KHZ][SPIOUT_CH_NUM][AD_CH_NUM],
	const float in_spiout_base_signal[SAMPLING_KHZ][SPIOUT_CH_NUM],
	ui16_t out_spiout_signal[SAMPLING_KHZ][SPIOUT_CH_NUM]
)
{
	float signal;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t to_ch = 0; to_ch < SPIOUT_CH_NUM; to_ch++ )
		{
			signal = in_spiout_base_signal[smp][to_ch];
			for( ui32_t from_ch = 0; from_ch < AD_CH_NUM; from_ch++ )
			{
				signal += in_ad_to_spiout_signal[smp][to_ch][from_ch] * ctrl_user_data_from_pc_1.from_ad_to_spiout_gain[to_ch][from_ch];
			}
			if( signal > 32767 )
			{
				signal = 65535;
			}
			else if( signal < -32768 )
			{
				signal = 0;
			}
			else
			{
				signal += 32768;
			}
			out_spiout_signal[smp][to_ch] = (ui16_t)signal;
		}
	}
}

static void ctrl_set_square_wave(
	ctrl_square_wave_info_st* out_square_wave_info_p
)
{
	out_square_wave_info_p->numerator_cycle = ctrl_user_data_from_pc_0.square_wave_numerator_cycle;
	out_square_wave_info_p->denominator_cycle = ctrl_user_data_from_pc_0.square_wave_denominator_cycle;
}

static void ctrl_set_spi_send_data(
	const ui16_t in_spiout_val[SAMPLING_KHZ][SPIOUT_CH_NUM],
	ctrl_spi_info_st* out_spi_info_p
)
{
	si32_t pt = 0;
	ui32_t ch_flag;

	for (si32_t smp = 0; smp < SAMPLING_KHZ; smp++)
	{
		out_spi_info_p->send_num[smp] = SPIOUT_CH_NUM / 2;
		for (si32_t ch = 0; ch < SPIOUT_CH_NUM; ch = ch + 2)
		{
			ui32_t tmp;

			ch_flag = ( ( ch / 2 ) + 1 ) << 12;
			tmp = ch_flag | ( ( in_spiout_val[smp][ch + 0] & 0xFFF0u ) >> 4 );
			tmp = tmp << 16;
			tmp |= ch_flag | ( ( in_spiout_val[smp][ch + 1] & 0xFFF0u ) >> 4 );
			out_spi_info_p->send_buf[pt] = tmp;
			out_spi_info_p->cs_keep_flag[pt] = false;
			pt++;
		}
	}
}

static void ctrl_set_can_send_data(
	ctrl_can_info_st* out_can_info_p
)
{
	out_can_info_p->send_request_num = ctrl_user_data_from_pc_0.can_send_num;
	memcpy( out_can_info_p->can_send_data, ctrl_user_data_from_pc_0.can_send_data, sizeof(ctrl_can_format_st) * CAN_MAX_FIFO_NUM );
	ctrl_user_data_from_pc_0.can_send_num = 0;
}

static void ctrl_set_can_status_to_pc(
	const ctrl_can_info_st* in_can_info_p,
	ctrl_can_format_st out_recv_data_p[CAN_MAX_FIFO_NUM],
	ctrl_can_format_st out_send_fin_data_p[CAN_MAX_FIFO_NUM],
	ui32_t* out_recv_num_p,
	ui32_t* out_send_fin_num_p,
	ui32_t* out_send_possible_num_p,
	ui32_t* out_cerror_p,
	ui32_t* out_cstatus_p
)
{
	*out_recv_num_p = in_can_info_p->recv_num;
	*out_send_fin_num_p = in_can_info_p->send_fin_num;
	if( in_can_info_p->send_possible_num > ctrl_user_data_from_pc_0.can_send_num )
	{
		*out_send_possible_num_p = in_can_info_p->send_possible_num - ctrl_user_data_from_pc_0.can_send_num;
	}
	else
	{
		*out_send_possible_num_p = 0u;
	}
	*out_cerror_p = in_can_info_p->cerror;
	*out_cstatus_p = in_can_info_p->cstatus;
	memcpy( out_recv_data_p, in_can_info_p->can_recv_data, sizeof(ctrl_can_format_st) * CAN_MAX_FIFO_NUM );
	memcpy( out_send_fin_data_p, in_can_info_p->can_send_fin_data, sizeof(ctrl_can_format_st) * CAN_MAX_FIFO_NUM );
}

// Warning:
// 1. Should not change top function address.
extern "C" void ctrl_generate_data( const ui32_t in_if_version, ctrl_io_data_st* io_data_p ) __attribute((section("TOP")));
void ctrl_generate_data( const ui32_t in_if_version, ctrl_io_data_st* io_data_p )
{
	float pwm_base_signal[SAMPLING_KHZ][PWM_CH_NUM];
	float spiout_base_signal[SAMPLING_KHZ][SPIOUT_CH_NUM];
	float ad_dccut_signal[SAMPLING_KHZ][AD_CH_NUM];
	ui16_t ad_dccut_val_for_send[SAMPLING_KHZ][AD_CH_NUM];

	float ad_to_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM][AD_CH_NUM];
	float ad_to_spiout_signal[SAMPLING_KHZ][SPIOUT_CH_NUM][AD_CH_NUM];
	float spiout_to_pwm_signal[SAMPLING_KHZ][PWM_CH_NUM][SPIOUT_CH_NUM];

	ui16_t spiout_val[SAMPLING_KHZ][SPIOUT_CH_NUM];

	if( in_if_version != IF_VERSION )
	{
		return;
	}
	if( io_data_p->first_flag )
	{
		ctrl_init();
	}
	else
	{
		ctrl_copy_param( io_data_p );
		ctrl_generate_random();
		ctrl_generate_pwm_base_signal( pwm_base_signal );
		ctrl_generate_spiout_base_signal( spiout_base_signal );
		ctrl_adapt_hpf( io_data_p->ad_val, ad_dccut_signal, ad_dccut_val_for_send );

		ctrl_set_delay_buf( ad_dccut_signal, spiout_base_signal );
		ctrl_get_delay_buf( ad_to_pwm_signal, ad_to_spiout_signal, spiout_to_pwm_signal );

		ctrl_generate_pwm_signal( ad_to_pwm_signal, spiout_to_pwm_signal, pwm_base_signal, io_data_p->pwm_val );
		ctrl_generate_spiout_val( ad_to_spiout_signal, spiout_base_signal, spiout_val );

		io_data_p->gpio_val = ctrl_user_data_from_pc_0.port_out_val;
		ctrl_set_square_wave(
			&io_data_p->square_wave_info
		);
		ctrl_set_spi_send_data(
			spiout_val,
			&io_data_p->spi_info
		);
		ctrl_set_can_send_data(
			&io_data_p->can_info
		);
		io_data_p->user_data_to_pc.user_struct_data.timestamp = ctrl_timestamp;
		memcpy( io_data_p->user_data_to_pc.user_struct_data.ad_val, ad_dccut_val_for_send, sizeof(ad_dccut_val_for_send) );
		memcpy( io_data_p->user_data_to_pc.user_struct_data.pwm_val, io_data_p->pwm_val, sizeof(io_data_p->pwm_val) );
		memcpy( io_data_p->user_data_to_pc.user_struct_data.spiout_val, spiout_val, sizeof(spiout_val) );
		ctrl_set_can_status_to_pc(
			&io_data_p->can_info,
			io_data_p->user_data_to_pc.user_struct_data.can_recv_data,
			io_data_p->user_data_to_pc.user_struct_data.can_send_fin_data,
			&io_data_p->user_data_to_pc.user_struct_data.can_recv_num,
			&io_data_p->user_data_to_pc.user_struct_data.can_send_fin_num,
			&io_data_p->user_data_to_pc.user_struct_data.can_send_possible_num,
			&io_data_p->user_data_to_pc.user_struct_data.cerror,
			&io_data_p->user_data_to_pc.user_struct_data.cstatus
		);
	}
	ctrl_timestamp++;
}
