`timescale 1ns / 1ps
`default_nettype none
/*
 doom_video.v

 The picture of Doom: the 320x200 frame of 8-bit colours in the SRAM, from one of two display buffers
 (FB0_HW / FB1_HW, halfword addresses): disp_front (clk_sys, doom_soc: the buffer with the last whole
 frame) is taken at the start of the vertical sync (disp_shown, back to clk_sys), so a frame is never
 changed while it is shown;
 doubled to 640x400 in the middle of 640x480 at 60 Hz (clk_pix 25 MHz, 800 x 525, syncs high active
 (the OSD overlay wants them so; the top makes them negative for HDMI) and always present, de the 640x480 window; the 40 lines above and below are black).

 Lines: source line L is shown on rows 40 + 2L and 41 + 2L; it is fetched into line buffer L & 1 while
 row 38 + 2L begins (a toggle with L to clk_sys: doom_sram vid_* port, 160 halfwords). The other buffer
 holds the line being shown. Palette: 256 x 24 bit (pal_* from the MMIO, clk_sys), read by the colour
 index. show = 0 (the CPU not running): black. Output: 5 clocks after the counters, with its syncs.
*/
module doom_video #(
	parameter [20:0] FB0_HW = 21'h1f0000,    // byte 0x3e0000
	parameter [20:0] FB1_HW = 21'h1e8000     // byte 0x3d0000
) (
	input wire clk_sys,
	output reg vid_req = 1'b0,
	output reg [20:0] vid_adr = 21'd0,
	output wire [8:0] vid_len,
	input wire vid_we,
	input wire [8:0] vid_wa,
	input wire [15:0] vid_wd,
	input wire pal_we,
	input wire [7:0] pal_wa,
	input wire [23:0] pal_wd,
	input wire show,                       // clk_sys
	input wire disp_front,                 // clk_sys
	output wire disp_shown,                // clk_sys

	input wire clk_pix,
	output reg [23:0] rgb = 24'd0,
	output reg hs = 1'b0,
	output reg vs = 1'b0,
	output reg de = 1'b0
);

localparam H_TOTAL = 800, H_VIS = 640, H_SYNC0 = 656, H_SYNC1 = 752;
localparam V_TOTAL = 525, V_VIS = 480, V_SYNC0 = 490, V_SYNC1 = 492;
localparam V_PIC0 = 40, V_PIC1 = 440;

assign vid_len = 9'd160;

// ------------------------------------------------------------------ counters (clk_pix)

reg [9:0] h = 10'd0, v = 10'd0;
reg req_t = 1'b0;                          // toggled for each line to fetch
reg [7:0] req_line = 8'd0;
wire front_p;
kg_sync #(.W(1)) sync_front (.clk(clk_pix), .d(disp_front), .q(front_p));
reg shown_p = 1'b0;
always @(posedge clk_pix) begin
	if (h == 10'd0 && v == V_SYNC0) shown_p <= front_p;
	if (h == H_TOTAL - 1) begin
		h <= 10'd0;
		v <= (v == V_TOTAL - 1) ? 10'd0 : v + 10'd1;
	end else
		h <= h + 10'd1;
	// the fetch of line L = (v - 38) / 2 at the start of row 38 + 2L
	if (h == 10'd0 && v >= V_PIC0 - 2 && v < V_PIC1 - 2 && !v[0]) begin
		req_line <= (v - (V_PIC0 - 2)) >> 1;
		req_t <= ~req_t;
	end
end

// ------------------------------------------------------------------ fetch (clk_sys)

wire req_s;
kg_sync #(.W(1)) sync_req (.clk(clk_sys), .d(req_t), .q(req_s));
kg_sync #(.W(1)) sync_shown (.clk(clk_sys), .d(shown_p), .q(disp_shown));
reg req_p = 1'b0;
reg buf_w = 1'b0;                          // the buffer of the line being fetched
always @(posedge clk_sys) begin
	req_p <= req_s;
	vid_req <= 1'b0;
	if (req_s != req_p) begin
		// req_line is stable: it changed with the toggle two clk_sys syncs ago
		vid_req <= 1'b1;
		vid_adr <= (disp_shown ? FB1_HW : FB0_HW) + { 13'd0, req_line } * 21'd160;
		buf_w <= req_line[0];
	end
end

wire [15:0] lb_q;
reg [8:0] lb_ra = 9'd0;
kg_sdpram #(.DW(16), .AW(9)) linebuf (
	.clka(clk_sys), .wea(vid_we), .addra({ buf_w, vid_wa[7:0] }), .dina(vid_wd),
	.clkb(clk_pix), .enb(1'b1), .addrb(lb_ra), .doutb(lb_q)
);

wire [23:0] pal_q;
reg [7:0] pal_ra = 8'd0;
kg_sdpram #(.DW(24), .AW(8)) palette (
	.clka(clk_sys), .wea(pal_we), .addra(pal_wa), .dina(pal_wd),
	.clkb(clk_pix), .enb(1'b1), .addrb(pal_ra), .doutb(pal_q)
);

wire show_p;
kg_sync #(.W(1)) sync_show (.clk(clk_pix), .d(show), .q(show_p));

// ------------------------------------------------------------------ pixels (clk_pix)

// edge n: the line buffer address of (h, v); n+1: the halfword; n+2: the palette address; n+3: the
// colour; n+4: out (the syncs and de through 4 stages as well)
wire pic0 = h < H_VIS && v >= V_PIC0 && v < V_PIC1;
wire [7:0] line0 = (v - V_PIC0) >> 1;
reg [3:0] pic = 4'd0, vis = 4'd0, hsd = 4'd0, vsd = 4'd0;
reg [1:0] bsel = 2'd0;
always @(posedge clk_pix) begin
	lb_ra <= { line0[0], h[9:2] };
	bsel <= { bsel[0], h[1] };
	pal_ra <= bsel[1] ? lb_q[15:8] : lb_q[7:0];
	pic <= { pic[2:0], pic0 };
	vis <= { vis[2:0], h < H_VIS && v < V_VIS };
	hsd <= { hsd[2:0], h >= H_SYNC0 && h < H_SYNC1 };
	vsd <= { vsd[2:0], v >= V_SYNC0 && v < V_SYNC1 };
	rgb <= (pic[3] && show_p) ? pal_q : 24'd0;
	de <= vis[3];
	hs <= hsd[3];
	vs <= vsd[3];
end

endmodule
