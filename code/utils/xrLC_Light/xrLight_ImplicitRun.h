#pragma once

class ImplicitDeflector;
void RunImplicitMultithread(ImplicitDeflector& defl);

namespace lc_net
{
	void RunImplicitnet(ImplicitDeflector& defl,  const xr_vector<u32> &exept );
}