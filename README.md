# Test DX11 
DX 11 d1str4ught client 

It has some visual bugs, I'm still working on it.
I'll fix them in time after I remove the directx9 to directx11 conversions.
I can log into the game without any problems.

This is just the beginning, don't start with empty words.

I removed ID3D11InputLayout and D3D11_INPUT_ELEMENT_DESC from the code (replaced with ID3D11ShaderReflection), 
sending them from the shader directly to the code.
