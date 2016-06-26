extern const u8 tile_shbin_end[];
extern const u8 tile_shbin[];
extern const u32 tile_shbin_size;


/*


	mov r0, c9
	mov r0.y, v0.w				; r0 = (8.0, height, 0, 0)
	mov r1, v1.z				; flip attribute
	mov r1, c5[r1]				; r1 now stores the final texture coordinate increments for flipping 
	
	; x1 y1
	setemit 0
	add outpos.xyz, v0.xyz, r0.zzz	; r0.z always = 0
	
	add outtex.xy, v1.xy, r1.xy
	emit

	; x2 y1
	setemit 1
	add outpos.xyz, v0.xyz, r0.xzz	; r0.z always = 0
	
	add outtex.xy, v1.xy, r1.zy
	emit

	; x1 y2
	setemit 2, prim
	add outpos.xyz, v0.xyz, r0.zyz	; r0.z always = 0
	
	add outtex.xy, v1.xy, r1.xw
	emit

	; x2 y2
	setemit 0, prim inv
	add outpos.xyz, v0.xyz, r0.xyz	; r0.z always = 0
	
	add outtex.xy, v1.xy, r1.zw
	emit
*/