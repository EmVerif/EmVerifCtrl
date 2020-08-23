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
	ctrl_timestamp = 0;
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

static void ctrl_generate_pwm_sine( ui16_t out_pwm_sine[SAMPLING_KHZ][PWM_CH_NUM] )
{
	si32_t sine;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t ch = 0; ch < PWM_CH_NUM; ch++ )
		{
			sine = 0;
			for( ui32_t idx = 0; idx < SINE_BASE_NUM; idx++ )
			{
				sine += 32768 * sin( ctrl_pwm_sine_current_phase[ch][idx] + ctrl_user_data_from_pc_0.pwm_sine_phase[ch][idx] ) * ctrl_user_data_from_pc_0.pwm_sine_gain[ch][idx];
				ctrl_pwm_sine_current_phase[ch][idx] += ( ctrl_user_data_from_pc_0.pwm_sine_hz[ch][idx] * ctrl_phase_step_per_hz );
				if( ctrl_pwm_sine_current_phase[ch][idx] >= ( 2 * ctrl_pi ) )
				{
					ctrl_pwm_sine_current_phase[ch][idx] -= ( 2 * ctrl_pi );
				}
			}
			if( sine > 32767 )
			{
				sine = 65535;
			}
			else if( sine < -32768 )
			{
				sine = 0;
			}
			else
			{
				sine += 32768;
			}
			out_pwm_sine[smp][ch] = (ui16_t)sine;
		}
	}
}

static void ctrl_generate_spiout_sine( ui16_t out_spiout_sine[SAMPLING_KHZ][SPIOUT_CH_NUM] )
{
	si32_t sine;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t ch = 0; ch < SPIOUT_CH_NUM; ch++ )
		{
			sine = 0;
			for( ui32_t idx = 0; idx < SINE_BASE_NUM; idx++ )
			{
				sine += 32768 * sin( ctrl_spiout_sine_current_phase[ch][idx] + ctrl_user_data_from_pc_0.spiout_sine_phase[ch][idx] ) * ctrl_user_data_from_pc_0.spiout_sine_gain[ch][idx];
				ctrl_spiout_sine_current_phase[ch][idx] += ( ctrl_user_data_from_pc_0.spiout_sine_hz[ch][idx] * ctrl_phase_step_per_hz );
				if( ctrl_spiout_sine_current_phase[ch][idx] >= ( 2 * ctrl_pi ) )
				{
					ctrl_spiout_sine_current_phase[ch][idx] -= ( 2 * ctrl_pi );
				}
			}
			if( sine > 32767 )
			{
				sine = 65535;
			}
			else if( sine < -32768 )
			{
				sine = 0;
			}
			else
			{
				sine += 32768;
			}
			out_spiout_sine[smp][ch] = (ui16_t)sine;
		}
	}
}

static void ctrl_generate_pwm_val(
	const ui16_t in_ad_val[SAMPLING_KHZ][AD_CH_NUM],
	const ui16_t in_spiout_sine[SAMPLING_KHZ][SPIOUT_CH_NUM],
	const ui16_t in_pwm_sine[SAMPLING_KHZ][PWM_CH_NUM],
	ui16_t out_pwm_val[SAMPLING_KHZ][PWM_CH_NUM]
)
{
	si32_t val;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t to_ch = 0; to_ch < PWM_CH_NUM; to_ch++ )
		{
			val = in_pwm_sine[smp][to_ch] - 32768;
			for( ui32_t from_ch = 0; from_ch < AD_CH_NUM; from_ch++ )
			{
				val += ( in_ad_val[smp][from_ch] - 32768 ) * ctrl_user_data_from_pc_1.from_ad_to_pwm_gain[to_ch][from_ch];
			}
			for( ui32_t from_ch = 0; from_ch < SPIOUT_CH_NUM; from_ch++ )
			{
				val += ( in_spiout_sine[smp][from_ch] - 32768 ) * ctrl_user_data_from_pc_1.from_spiout_to_pwm_gain[to_ch][from_ch];
			}
			if( val > 32767 )
			{
				val = 65535;
			}
			else if( val < -32768 )
			{
				val = 0;
			}
			else
			{
				val += 32768;
			}
			out_pwm_val[smp][to_ch] = (ui16_t)val;
		}
	}
}

static void ctrl_generate_spiout_val(
	const ui16_t in_ad_val[SAMPLING_KHZ][AD_CH_NUM],
	const ui16_t in_spiout_sine[SAMPLING_KHZ][SPIOUT_CH_NUM],
	ui16_t out_spiout_val[SAMPLING_KHZ][SPIOUT_CH_NUM]
)
{
	si32_t val;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t to_ch = 0; to_ch < SPIOUT_CH_NUM; to_ch++ )
		{
			val = in_spiout_sine[smp][to_ch] - 32768;
			for( ui32_t from_ch = 0; from_ch < AD_CH_NUM; from_ch++ )
			{
				val += ( in_ad_val[smp][from_ch] - 32768 ) * ctrl_user_data_from_pc_1.from_ad_to_spiout_gain[to_ch][from_ch];
			}
			if( val > 32767 )
			{
				val = 65535;
			}
			else if( val < -32768 )
			{
				val = 0;
			}
			else
			{
				val += 32768;
			}
			out_spiout_val[smp][to_ch] = (ui16_t)val;
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
	ui16_t spiout_sine[SAMPLING_KHZ][SPIOUT_CH_NUM];
	ui16_t pwm_sine[SAMPLING_KHZ][PWM_CH_NUM];
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
		ctrl_generate_pwm_sine( pwm_sine );
		ctrl_generate_spiout_sine( spiout_sine );
		ctrl_generate_pwm_val( io_data_p->ad_val, spiout_sine, pwm_sine, io_data_p->pwm_val );
		ctrl_generate_spiout_val( io_data_p->ad_val, spiout_sine, spiout_val );

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
		memcpy( io_data_p->user_data_to_pc.user_struct_data.ad_val, io_data_p->ad_val, sizeof(io_data_p->ad_val) );
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
