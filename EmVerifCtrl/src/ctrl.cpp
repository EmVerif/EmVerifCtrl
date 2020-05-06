#include <math.h>
#include <string.h>
#include "typedefine.h"
#include "ctrl.h"

// TODO: Sample program
static ctrl_user_data_from_pc_st ctrl_param __attribute((aligned(32)));
static float ctrl_current_phase[THROUGH_OUT_CH_NUM][NOISE_BASE_NUM];
static ui32_t ctrl_timestamp;

static const float ctrl_pi = 3.14159265358979323846;
static const float ctrl_phase_step_per_hz = ( 2.0 * ctrl_pi ) / SAMPLING_KHZ / 1000;


static void ctrl_init( void )
{
	memset( (void*)&ctrl_param, 0, sizeof(ctrl_param) );
	for( ui32_t ch = 0; ch < THROUGH_OUT_CH_NUM; ch++ )
	{
		for( ui32_t idx = 0; idx < NOISE_BASE_NUM; idx++ )
		{
			ctrl_current_phase[ch][idx] = 0.0;
		}
	}
	ctrl_timestamp = 0;
}

static void ctrl_copy_param( ctrl_io_data_st* io_data_p )
{
	if( io_data_p->user_data_from_pc_valid )
	{
		ctrl_param = io_data_p->user_data_from_pc.user_struct_data;
	}
}

static void ctrl_generate_through_out_val( ui16_t out_through_out_val[SAMPLING_KHZ][THROUGH_OUT_CH_NUM] )
{
	si32_t sig_val;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t ch = 0; ch < THROUGH_OUT_CH_NUM; ch++ )
		{
			sig_val = 0;
			for( ui32_t idx = 0; idx < NOISE_BASE_NUM; idx++ )
			{
				sig_val += 32768 * sin( ctrl_current_phase[ch][idx] + ctrl_param.sine_phase[ch][idx] ) * ctrl_param.sine_gain[ch][idx];
				ctrl_current_phase[ch][idx] += ( ctrl_param.sine_hz[ch][idx] * ctrl_phase_step_per_hz );
				if( ctrl_current_phase[ch][idx] >= ( 2 * ctrl_pi ) )
				{
					ctrl_current_phase[ch][idx] -= ( 2 * ctrl_pi );
				}
			}
			if( sig_val > 32767 )
			{
				sig_val = 65535;
			}
			else if( sig_val < -32768 )
			{
				sig_val = 0;
			}
			else
			{
				sig_val += 32768;
			}
			out_through_out_val[smp][ch] = (ui16_t)sig_val;
		}
	}
}

static void ctrl_generate_mix_out_val(
	const ui16_t in_in_val[SAMPLING_KHZ][IN_CH_NUM],
	const ui16_t in_through_out_val[SAMPLING_KHZ][THROUGH_OUT_CH_NUM],
	ui16_t out_mix_out_val[SAMPLING_KHZ][MIX_OUT_CH_NUM]
)
{
	si32_t sig_val;

	for( si32_t smp = 0; smp < SAMPLING_KHZ; smp++ )
	{
		for( ui32_t to_ch = 0; to_ch < MIX_OUT_CH_NUM; to_ch++ )
		{
			sig_val = 0;
			for( ui32_t from_ch = 0; from_ch < THROUGH_OUT_CH_NUM; from_ch++ )
			{
				sig_val += ( in_through_out_val[smp][from_ch] - 32768 ) * ctrl_param.from_through_out_to_mix_out_gain[to_ch][from_ch];
			}
			for( ui32_t from_ch = 0; from_ch < IN_CH_NUM; from_ch++ )
			{
				sig_val += ( in_in_val[smp][from_ch] - 32768 ) * ctrl_param.from_in_to_mix_out_gain[to_ch][from_ch];
			}
			if( sig_val > 32767 )
			{
				sig_val = 65535;
			}
			else if( sig_val < -32768 )
			{
				sig_val = 0;
			}
			else
			{
				sig_val += 32768;
			}
			out_mix_out_val[smp][to_ch] = (ui16_t)sig_val;
		}
	}
}

static void ctrl_set_pwm(
	const ui16_t in_mix_out_val[SAMPLING_KHZ][MIX_OUT_CH_NUM],
	ui16_t out_pwm_val[SAMPLING_KHZ][PWM_CH_NUM]
)
{
	si32_t pt = 0;

	for (si32_t smp = 0; smp < SAMPLING_KHZ; smp++)
	{
		for (si32_t ch = 0; ch < MIX_OUT_CH_NUM; ch++)
		{
			out_pwm_val[smp][ch] = in_mix_out_val[smp][ch];
		}
	}
}

static void ctrl_set_spi_send_data(
	const ui16_t in_through_out_val[SAMPLING_KHZ][THROUGH_OUT_CH_NUM],
	ctrl_spi_info_st* out_spi_info_p
)
{
	si32_t pt = 0;
	ui32_t ch_flag;

	for (si32_t smp = 0; smp < SAMPLING_KHZ; smp++)
	{
		out_spi_info_p->send_num[smp] = THROUGH_OUT_CH_NUM;
		for (si32_t ch = 0; ch < THROUGH_OUT_CH_NUM; ch = ch + 2)
		{
			ch_flag = ( ( ch / 2 ) + 1 ) << 12;
			out_spi_info_p->send_buf[pt + 0] = ch_flag | ( ( in_through_out_val[smp][ch + 0] & 0xFFF0u ) >> 4 );
			out_spi_info_p->send_buf[pt + 1] = ch_flag | ( ( in_through_out_val[smp][ch + 1] & 0xFFF0u ) >> 4 );
			out_spi_info_p->cs_keep_flag[pt + 0] = true;
			out_spi_info_p->cs_keep_flag[pt + 1] = false;
			pt += 2;
		}
	}
}

static void ctrl_set_can_send_data(
	ctrl_can_info_st* out_can_info_p
)
{
	out_can_info_p->send_request_num = ctrl_param.can_send_num;
	for (ui32_t idx = 0; idx < ctrl_param.can_send_num; idx++)
	{
		memcpy(out_can_info_p->can_send_data, ctrl_param.can_send_data, sizeof(ctrl_can_format_st) * CAN_MAX_FIFO_NUM);
	}
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
	if( in_can_info_p->send_possible_num > ctrl_param.can_send_num )
	{
		*out_send_possible_num_p = in_can_info_p->send_possible_num - ctrl_param.can_send_num;
	}
	else
	{
		*out_send_possible_num_p = 0u;
	}
	*out_cerror_p = in_can_info_p->cerror;
	*out_cstatus_p = in_can_info_p->cstatus;
	for( ui32_t idx = 0; idx < *out_recv_num_p; idx++ )
	{
		out_recv_data_p[idx] = in_can_info_p->can_recv_data[idx];
	}
	for( ui32_t idx = 0; idx < *out_send_fin_num_p; idx++ )
	{
		out_send_fin_data_p[idx] = in_can_info_p->can_send_fin_data[idx];
	}
}

// Warning:
// 1. Should not change top function address.
extern "C" void ctrl_generate_data( const ui32_t in_if_version, ctrl_io_data_st* io_data_p ) __attribute((section("TOP")));
void ctrl_generate_data( const ui32_t in_if_version, ctrl_io_data_st* io_data_p )
{
	ui16_t through_out_val[SAMPLING_KHZ][THROUGH_OUT_CH_NUM];
	ui16_t mix_out_val[SAMPLING_KHZ][MIX_OUT_CH_NUM];

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
		ctrl_generate_through_out_val( through_out_val );
		ctrl_generate_mix_out_val(
			io_data_p->ad_val,
			through_out_val,
			mix_out_val
		);
		ctrl_set_pwm(
			mix_out_val,
			io_data_p->pwm_val
		);
		ctrl_set_spi_send_data(
			through_out_val,
			&io_data_p->spi_info[0]
		);
		ctrl_set_can_send_data(
			&io_data_p->can_info
		);

		io_data_p->user_data_to_pc.user_struct_data.timestamp = ctrl_timestamp;
		memcpy( io_data_p->user_data_to_pc.user_struct_data.in_val, io_data_p->ad_val, sizeof(io_data_p->ad_val) );
		memcpy( io_data_p->user_data_to_pc.user_struct_data.mix_out_val, mix_out_val, sizeof(mix_out_val) );
		memcpy( io_data_p->user_data_to_pc.user_struct_data.through_out_val, through_out_val, sizeof(through_out_val) );
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
