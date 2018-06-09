#include "thumbsim.hpp"
// These are just the register NUMBERS
#define PC_REG 15  
#define LR_REG 14
#define SP_REG 13

// These are the contents of those registers
#define PC rf[PC_REG]
#define LR rf[LR_REG]
#define SP rf[SP_REG]

Stats stats;
Caches caches(0);

// CPE 315: you'll need to implement a custom sign-extension function
// in addition to the ones given below, specifically for the unconditional
// branch instruction, which has an 11-bit immediate field
unsigned int signExtend16to32ui(short i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend8to32ui(char i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}
int signExtend11to32ui(unsigned short i) {
	//cout << "=============================== " << i << endl;
	if(i >= 1024)
		cout << "==========+++====++++======++++================" << endl;
	unsigned char mask;
	if(i >= 128)
	cout << "signextend=======: " << (unsigned int)i << endl;
	
	mask = 1;
    mask = mask << 7;

    if(mask & i)
    	cout << "We got a neg" << endl;

  return static_cast<unsigned int>(static_cast<int>(i));
}

// This is the global object you'll use to store condition codes N,Z,V,C
// Set these bits appropriately in execute below.
ASPR flags;

// CPE 315: You need to implement a function to set the Negative and Zero
// flags for each instruction that does that. It only needs to take
// one parameter as input, the result of whatever operation is executing
void setNegativeZero(int result){
	//cout << "Checking: " << result << " to be negative or 0" << endl;
	if((long)result == 0) {
		flags.Z = 1;
		//cout << "Flags.Z = 1" << endl;
		return;
	}
	else {
		//cout << "Flags.Z = 0" << endl;
		flags.Z = 0;
	}

	if((long)result < 0) {
		//cout << "Flags.N = 1" << endl;
		flags.N = 1;
		return;
	}
	else if ((long)result > 0)
	{
		//cout << "Flags.N = 0" << endl;
		flags.N = 0;
	}
	//not done yet ============================================================================================================
}

// This function is complete, you should not have to modify it
void setCarryOverflow (int num1, int num2, OFType oftype) {
  switch (oftype) {
    case OF_ADD:
      if (((unsigned long long int)num1 + (unsigned long long int)num2) ==
          ((unsigned int)num1 + (unsigned int)num2)) {
        flags.C = 0;
      }
      else {
        flags.C = 1;
      }
      if (((long long int)num1 + (long long int)num2) ==
          ((int)num1 + (int)num2)) {
        flags.V = 0;
      }
      else {
        flags.V = 1;
      }
      break;
    case OF_SUB:
      if (num1 >= num2) {
        flags.C = 1;
      }
      else if (((unsigned long long int)num1 - (unsigned long long int)num2) ==
          ((unsigned int)num1 - (unsigned int)num2)) {
        flags.C = 0;
      
      }
      else {
        flags.C = 1;
      }
      if (((num1==0) && (num2==0)) ||
          (((long long int)num1 - (long long int)num2) ==
           ((int)num1 - (int)num2))) {
        flags.V = 0;
      }
      else {
        flags.V = 1;
      }
      break;
    case OF_SHIFT:
      // C flag unaffected for shifts by zero
      if (num2 != 0) {
        if (((unsigned long long int)num1 << (unsigned long long int)num2) ==
            ((unsigned int)num1 << (unsigned int)num2)) {
          flags.C = 0;
        }
        else {
          flags.C = 1;
        }
      }
      // Shift doesn't set overflow
      break;
    default:
      cerr << "Bad OverFlow Type encountered." << __LINE__ << __FILE__ << endl;
      exit(1);
  }
}

// CPE 315: You're given the code for evaluating BEQ, and you'll need to 
// complete the rest of these conditions. See Page 208 of the armv7 manual
static int checkCondition(unsigned short cond) {
	//stats.numRegReads += 1;
  switch(cond) {
    case EQ:
      if (flags.Z == 1) {
        return TRUE;
      }
      break;
    case NE:
      if (flags.Z == 0) {
      	return TRUE;
      }     
      break;
    case CS:
      if (flags.C == 1) {
      	return TRUE;
      }
      break;
    case CC:
    if (flags.C == 0) {
      	return TRUE;
      }
      break;
    case MI:
    if (flags.N == 1) {
      	return TRUE;
      }
      break;
    case PL:
    if (flags.N == 0) {
      	return TRUE;
      }
      break;
    case VS:
    if (flags.V == 1) {
      	return TRUE;
      }
      break;
    case VC:
    if (flags.V == 0) {
      	return TRUE;
      }
      break;
    case HI:
    if ((flags.C == 1) && (flags.Z == 0)) {
      	return TRUE;
      }
      break;
    case LS:
    if ((flags.C == 0) || (flags.Z == 1)) {
      	return TRUE;
      }
      break;
    case GE:
    if (flags.N == flags.V) {
      	return TRUE;
      }
      break;
    case LT:
    if (flags.N != flags.V) {
      	return TRUE;
      }
      break;
    case GT:
    if ((flags.Z == 0) && (flags.N == flags.V)) {
      	return TRUE;
      }
      break;
    case LE:
    if ((flags.Z == 1) || (flags.N != flags.V)) {
      	return TRUE;
      }
      break;
    case AL:
      return TRUE;
      break;
  }
  return FALSE;
}
int bitCount(unsigned int list){
	//whatever reg_list is s gonna be a map for which registers are psuhed / popped
	//so each of the eight bits represent a register, in this function
	//Read in the number, and go through and read each indavidual bit, if its a 1, 
	//add 1 to the bitcount, return bitCount
	int result = 0;
	while(list) {
		result += list%2;
		list>>=1;
	}
	//cout << "bitCount: " << result << endl;
	return result;
}

void execute() {
  Data16 instr = imem[PC];
  Data16 instr2;
  Data32 temp(0); // Use this for STRB instructions
  Thumb_Types itype;
  // the following counts as a read to PC
  unsigned int pctarget = PC + 2;
  unsigned int addr;
  int i, n, offset;
  unsigned int list, mask;
  int num1, num2, result, BitCount;
  unsigned int bit;

  /* Convert instruction to correct type */
  /* Types are described in Section A5 of the armv7 manual */
  BL_Type blupper(instr);
  ALU_Type alu(instr);
  SP_Type sp(instr);
  DP_Type dp(instr);
  LD_ST_Type ld_st(instr);
  MISC_Type misc(instr);
  COND_Type cond(instr);
  UNCOND_Type uncond(instr);
  LDM_Type ldm(instr);
  STM_Type stm(instr);
  LDRL_Type ldrl(instr);
  ADD_SP_Type addsp(instr);

  BL_Ops bl_ops;
  ALU_Ops add_ops;
  DP_Ops dp_ops;
  SP_Ops sp_ops;
  LD_ST_Ops ldst_ops;
  MISC_Ops misc_ops;

  // This counts as a write to the PC register
  rf.write(PC_REG, pctarget);
  //stats???
  stats.numRegWrites +=1 ;
  stats.numRegReads += 1;


  itype = decode(ALL_Types(instr));

  // CPE 315: The bulk of your work is in the following switch statement
  // All instructions will need to have stats and cache access info added
  // as appropriate for that instruction.
  switch(itype) {
    case ALU:
      add_ops = decode(alu);
      switch(add_ops) {
        case ALU_LSLI:
          //My first attempt at the code, gonna copy the one below
          //probably wront as im adding the reg and the immediate?
          rf.write(alu.instr.lsli.rd, rf[alu.instr.lsli.rm] << alu.instr.lsli.imm);
          

          setCarryOverflow(rf[alu.instr.lsli.rm], rf[alu.instr.lsli.imm], OF_SHIFT);
          setNegativeZero(rf[alu.instr.lsli.rm] << alu.instr.lsli.imm);
          
          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 2;
          stats.instrs++;

          break;
        case ALU_ADDR:
          // needs stats and flags
          rf.write(alu.instr.addr.rd, rf[alu.instr.addr.rn] + rf[alu.instr.addr.rm]);
          setCarryOverflow(rf[alu.instr.addr.rn], rf[alu.instr.addr.rm], OF_ADD);
          setNegativeZero(rf[alu.instr.addr.rn] + rf[alu.instr.addr.rm]);

          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 2;
          stats.instrs++;

          break;
        case ALU_SUBR:
          //Done by mee
          //how does any of this work???
          rf.write(alu.instr.subr.rd, rf[alu.instr.subr.rn] + rf[alu.instr.subr.rm]);
          setCarryOverflow(rf[alu.instr.subr.rn], rf[alu.instr.subr.rm], OF_SUB);
          setNegativeZero(rf[alu.instr.subr.rn] - rf[alu.instr.subr.rm]);
          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 2;
          stats.instrs++;

          break;
        case ALU_ADD3I:
          // needs stats and flags
          rf.write(alu.instr.add3i.rd, rf[alu.instr.add3i.rn] + alu.instr.add3i.imm);

          setCarryOverflow(rf[alu.instr.add3i.rn], alu.instr.add3i.imm, OF_ADD);
          setNegativeZero(rf[alu.instr.add3i.rn] + alu.instr.add3i.imm);

          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;

          break;
        case ALU_SUB3I:
          //done by me
          rf.write(alu.instr.sub3i.rd, rf[alu.instr.sub3i.rn] + alu.instr.sub3i.imm);

          setCarryOverflow(rf[alu.instr.sub3i.rn], alu.instr.sub3i.imm, OF_SUB);
          setNegativeZero(rf[alu.instr.sub3i.rn] - alu.instr.sub3i.imm);

          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;

          break;
        case ALU_MOV:
          // needs stats and flags
          rf.write(alu.instr.mov.rdn, alu.instr.mov.imm);
          //setCarryOverflow(rf[alu.instr.mov.rdn], alu.instr.mov.imm, OF_SHIFT);
          setNegativeZero(alu.instr.mov.imm);
          //setNegativeZero(rf[alu.instr.addr.rn] - alu.instr.addr.rm);
          //Stats
          stats.numRegWrites += 1;
          stats.instrs++;

          
          break;
        case ALU_CMP:
          setCarryOverflow(rf[alu.instr.cmp.rdn], alu.instr.cmp.imm, OF_SUB);
          setNegativeZero(rf[alu.instr.cmp.rdn] - alu.instr.cmp.imm);

		  //Stats	  
		  stats.numRegWrites += 1;
		  stats.numRegReads += 2;
		  stats.instrs++;
          
          break;
        case ALU_ADD8I:
          // needs stats and flags
          rf.write(alu.instr.add8i.rdn, rf[alu.instr.add8i.rdn] + alu.instr.add8i.imm);

          setCarryOverflow(rf[alu.instr.add8i.rdn], alu.instr.add8i.imm, OF_ADD);
          setNegativeZero(rf[alu.instr.add8i.rdn] + alu.instr.add8i.imm);

          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;
          break;
        case ALU_SUB8I:
          rf.write(alu.instr.sub8i.rdn, rf[alu.instr.sub8i.rdn] - alu.instr.sub8i.imm);
          setCarryOverflow(rf[alu.instr.sub8i.rdn], alu.instr.sub8i.imm, OF_SUB);
          setNegativeZero(rf[alu.instr.sub8i.rdn] - alu.instr.sub8i.imm);
          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;
          break;
        default:
          //cout << "instruction not implemented" << endl;
          exit(1);
          break;
      }
      break;
    case BL: 
      // This instruction is complete, nothing needed here
      bl_ops = decode(blupper);
      if (bl_ops == BL_UPPER) {
        // PC has already been incremented above
        instr2 = imem[PC];
        BL_Type bllower(instr2);
        if (blupper.instr.bl_upper.s) {
          addr = static_cast<unsigned int>(0xff<<24) | 
            ((~(bllower.instr.bl_lower.j1 ^ blupper.instr.bl_upper.s))<<23) |
            ((~(bllower.instr.bl_lower.j2 ^ blupper.instr.bl_upper.s))<<22) |
            ((blupper.instr.bl_upper.imm10)<<12) |
            ((bllower.instr.bl_lower.imm11)<<1);
        }
        else {
          addr = ((blupper.instr.bl_upper.imm10)<<12) |
            ((bllower.instr.bl_lower.imm11)<<1);
        }
        // return address is 4-bytes away from the start of the BL insn
        rf.write(LR_REG, PC + 2);
        // Target address is also computed from that point
        rf.write(PC_REG, PC + 2 + addr);

        stats.numRegReads += 1;
        stats.numRegWrites += 2;
        stats.instrs++;
 
      }
      else {
        cerr << "Bad BL format." << endl;
        exit(1);
      }
      break;
    case DP:
      dp_ops = decode(dp);
      switch(dp_ops) {
        case DP_CMP:

          setCarryOverflow(rf[dp.instr.DP_Instr.rdn], rf[dp.instr.DP_Instr.rm], OF_SUB);
          setNegativeZero(rf[dp.instr.DP_Instr.rdn] - rf[dp.instr.DP_Instr.rm]);

		  //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 4;
          stats.instrs++;

          break;
      }
      break;
    case SPECIAL:
      sp_ops = decode(sp);
      switch(sp_ops) {
        case SP_MOV:
          // needs stats and flags
          rf.write((sp.instr.mov.d << 3 ) | sp.instr.mov.rd, rf[sp.instr.mov.rm]);
           //setCarryOverflow(rf[sp.instr.mov.rm], alu.instr.mov.imm, OF_SHIFT);
          setNegativeZero(rf[sp.instr.mov.rm]);
          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;
          break;
        case SP_ADD:
       	  rf.write((sp.instr.add.d << 3) | sp.instr.add.rd, rf[((sp.instr.add.d << 3) | sp.instr.add.rd)] + rf[sp.instr.add.rm] );
       	  setCarryOverflow(rf[((sp.instr.add.d << 3) | sp.instr.add.rd)], rf[sp.instr.add.rm], OF_ADD);
          setNegativeZero(rf[((sp.instr.add.d << 3) | sp.instr.add.rd)] + rf[sp.instr.add.rm]);
       	  //stats
       	  stats.numRegWrites++;
       	  stats.numRegReads++;
       	  stats.instrs++;

       	  break; //===================================================THe break; was not there, added it myslef. pretty sure it goes there but idk
        case SP_CMP:
          // need to implement these
          cout << "Using SP CMP!!!!!!!!!!!!!!!!!!!!!!" <<  endl;

          setCarryOverflow(rf[sp.instr.cmp.rd], (rf[sp.instr.cmp.rm] << 3),OF_SUB);
          setNegativeZero(rf[sp.instr.cmp.rd] - (rf[sp.instr.cmp.rm] << 3) );
		  //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 4;
          stats.instrs++;

          break;
      }
      break;
    case LD_ST:
      // You'll want to use these load and store models
      // to implement ldrb/strb, ldm/stm and push/pop
      ldst_ops = decode(ld_st);
      switch(ldst_ops) {
        case STRI:
          // functionally complete, needs stats
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          dmem.write(addr, rf[ld_st.instr.ld_st_imm.rt]);
          //Stats
          stats.numRegReads += 2;
          stats.numMemWrites += 1;
          stats.instrs++;

          break;
        case LDRI:
          // functionally complete, needs stats
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          rf.write(ld_st.instr.ld_st_imm.rt, dmem[addr]);

           //Stats
          //cout << "Placing this number into register: " << dmem[addr] << endl;
          stats.numRegReads += 1;
          stats.numRegWrites += 1;
          stats.numMemReads += 1;
          stats.instrs++;
          break;
        case STRR:     
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          dmem.write(addr, rf[ld_st.instr.ld_st_reg.rt]);


		
         // cout << "Address: " << hex << addr << endl;
           //Stats
          stats.numRegReads += 2;
          stats.numMemWrites += 1;
          stats.instrs++;
          break;
        case LDRR:
          // need to implement
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
          rf.write(ld_st.instr.ld_st_reg.rt, dmem[addr]);
           //Stats
          stats.numRegReads += 1;
          stats.numRegWrites += 1;
          stats.numMemReads += 1;
          stats.instrs++;

          break;
        case STRBI:
          addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;
          temp = dmem[addr];

 		  temp.set_data_ubyte4(0,rf[ld_st.instr.ld_st_imm.rt]);

 		  //temp.set_data_ubyte4(ld_st.instr.ld_st_imm.imm % 4, temp);
          dmem.write(addr,temp);


          //Stats
          stats.numRegReads+=2;
          //stats.numMemReads+=1;
          stats.numMemWrites+=1;
          stats.instrs++;

          // need to implement

          break;
          //==========================================================
        case LDRBI:
            //cout << "Need to implement LDRBI" << endl;
          
           addr = rf[ld_st.instr.ld_st_imm.rn] + ld_st.instr.ld_st_imm.imm * 4;// + ld_st.instr.ld_st_imm.imm ;

        
           rf.write(ld_st.instr.ld_st_imm.rt, dmem[addr].data_ubyte4(0));

           //Stats
           stats.numRegReads++;
           stats.numRegWrites++;
           stats.numMemReads++;
           stats.instrs++;

        
          break;
        case STRBR:
          
          addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];

          temp = dmem[addr];
          //temp = rf[ld_st.instr.ld_st_imm.rt];
 		  temp.set_data_ubyte4( 0, rf[ld_st.instr.ld_st_reg.rt]);
 		  if(rf[ld_st.instr.ld_st_reg.rn] ==  SP)
               cout << "We got a big reg " << hex << addr << endl;

 		  //temp.set_data_ubyte4(ld_st.instr.ld_st_imm.imm % 4, temp);
          dmem.write(addr,temp);

          //Stats
           stats.numRegReads += 3;
           stats.numRegWrites++;
           stats.numMemReads++;
           stats.numMemWrites++;
           stats.instrs++;


          break;
        case LDRBR:

           addr = rf[ld_st.instr.ld_st_reg.rn] + rf[ld_st.instr.ld_st_reg.rm];
           
           //i = rf[ld_st.instr.ld_st_reg.rm];
           rf.write(ld_st.instr.ld_st_reg.rt, signExtend8to32ui(dmem[addr].data_ubyte4(0)));
           //thats how u do that boi
		   //Stats
           stats.numRegReads += 2;
           stats.numRegWrites++;
           stats.numMemReads++;
           stats.instrs++;
          break;
      }
      break;
    case MISC:
      misc_ops = decode(misc);
      switch(misc_ops) {
        case MISC_PUSH:
          // need to implement
          //Page # A8- 248i
          n = 16;
          list = (misc.instr.push.m<<(n-2)) | misc.instr.push.reg_list;
          addr = SP - 4*bitCount(list);          
          for(i=0, mask = 1; i<n; i++, mask <<=1){
            if(list & mask){
          		//Do this list with a mask 
 			   //Stats         	
            	
               caches.access(addr);
               //write to memmory
               dmem.write(addr, rf[i]);

               addr += 4;
               //Stats
               stats.numRegReads += 1;
               stats.numMemWrites += 1;
			   //stats.instrs++;

            }
          }
          if(misc.instr.push.m){

          }
          //Now store the SPs updated value in SP register
          rf.write(SP_REG, SP - 4 * bitCount(list));
           //Stats
          stats.numRegWrites += 1;
   		  stats.numRegReads += 1;
   		  stats.instrs++;

   		  //stats.instrs++;


          break;
        case MISC_POP:
          // need to implement
          n = 16;
          list = (misc.instr.pop.m<<(n-1)) | misc.instr.pop.reg_list;
          addr = SP;
          for(i=0, mask = 1; i<(n/2); i++, mask <<=1){
            if(mask & misc.instr.pop.reg_list){
          		//Do this list with a mask 
 			   //Stats         	
               caches.access(addr);
               //write to memmory
              
               
               rf.write(i, dmem[addr]);
               addr += 4;
               //Stats
               stats.numMemReads += 1;
               stats.numRegWrites += 1;
               //stats.instrs++;

            }

          }
         
		  if(misc.instr.pop.m){
             rf.write(PC_REG, dmem[addr]);
           	//stats
           	stats.numRegWrites += 1;
          	stats.numMemReads +=1;
          }
          

          rf.write(SP_REG, SP + 4 *bitCount(list));
           //Stats
          stats.numRegWrites += 1;

          //Maybe this needs ti be 2? its 2 for me but i ciuld resuse addres maybe im not sure
          stats.numRegReads += 1;
          stats.instrs++;

         // stats.instrs++;


          break;
        case MISC_SUB:
          // functionally complete, needs stats
          rf.write(SP_REG, SP - (misc.instr.sub.imm*4));
          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;

          break;
        case MISC_ADD:
          // functionally complete, needs stats
          rf.write(SP_REG, SP + (misc.instr.add.imm*4));
          //Stats
          stats.numRegWrites += 1;
          stats.numRegReads += 1;
          stats.instrs++;

          break;
      }
      break;
    case COND:
      decode(cond);
      //unsigned char mask;
      // Once you've completed the checkCondition function,
      // this should work for all your conditional branches.
      // needs stats
      stats.instrs++;
      if (checkCondition(cond.instr.b.cond)){
        rf.write(PC_REG, PC + 2 * signExtend8to32ui(cond.instr.b.imm) + 2);
       
        //Stats
        //stats.numRegWrites += 1;
        //stats.numRegReads += 1;
        //These should be true????
        

        //Backward branch taken
        if((int)signExtend8to32ui(cond.instr.b.imm)<= 0)
           stats.numBackwardBranchesTaken += 1;
       //Forward branc taken
       else if((int)signExtend8to32ui(cond.instr.b.imm) > 0 )
           stats.numForwardBranchesTaken += 1;
      }
      //Branch not taken, check if forward or backward
      else{
      	if((int)signExtend8to32ui(cond.instr.b.imm) > 0 )
           stats.numForwardBranchesNotTaken += 1;

       else if((int)signExtend8to32ui(cond.instr.b.imm) <= 0)
           stats.numBackwardBranchesNotTaken += 1;

      }
      break;
    case UNCOND:
      // Essentially the same as the conditional branches, but with no
      // condition check, and an 11-bit immediate field
      decode(uncond);
      if(uncond.instr.b.imm >= 1024 )
      	cout << "Faggot " << endl;
      	//cout << "Fuck my dude!!!! " << dec << signExtend11to32ui(cond.instr.b.imm) << endl;
      rf.write(PC_REG, PC + 2 * (int)signExtend11to32ui(uncond.instr.b.imm) + 2);
      
     // stats.numRegWrites += 1;
      //stats.numRegReads += 1;
      stats.instrs++;

      break;
    case LDM:
      decode(ldm);

 		n = 16;
          list = ldm.instr.ldm.reg_list;
          addr = ldm.instr.ldm.rn;
          for(i=0, mask = 1; i<n; i++, mask <<=1){
            if(mask & list){
          		//Do this list with a mask 
 			   //Stats         	
               caches.access(addr);
               //write to memmory
               rf.write(i, dmem[addr]);
               addr += 4;
               //Stats
               stats.numMemReads += 1;
               stats.numRegWrites += 1;
               //stats.instrs++;

            }

          }
             rf.write(ldm.instr.ldm.rn, dmem[addr]);
           	//stats
           	stats.numRegWrites += 1;
          	stats.numMemReads +=1;
        	

          rf.write(ldm.instr.ldm.rn, rf[ldm.instr.ldm.rn] + 4 *bitCount(list));




      // need to implement
      break;
    case STM:
      decode(stm);
 		  n = 16;
          list = stm.instr.stm.reg_list;

          addr = rf[stm.instr.stm.rn] - 4*bitCount(list);          
          for(i=0, mask = 1; i<n; i++, mask <<=1){
            if(list & mask){
          		//Do this list with a mask 
 			   //Stats         	
               caches.access(addr);
               //write to memmory
               dmem.write(addr, rf[i]);

               addr += 4;
               //Stats
               stats.numRegReads += 1;
               stats.numMemWrites += 1;
			   //stats.instrs++;

            }
          }
          //Now store the SPs updated value in SP register
          rf.write(stm.instr.stm.rn, rf[stm.instr.stm.rn] - 4 * bitCount(list));
           //Stats
          stats.numRegWrites += 1;
   		  stats.numRegReads += 1;
   		  stats.instrs++;



      // need to implement
      break;
    case LDRL:
      // This instruction is complete, nothing needed
      decode(ldrl);
      // Need to check for alignment by 4
      if (PC & 2) {
        addr = PC + 2 + (ldrl.instr.ldrl.imm)*4;
      }
      else {
        addr = PC + (ldrl.instr.ldrl.imm)*4;
      }
      // Requires two consecutive imem locations pieced together
      temp = imem[addr] | (imem[addr+2]<<16);  // temp is a Data32
      rf.write(ldrl.instr.ldrl.rt, temp);

      // One write for updated reg
      stats.numRegWrites++;
      // One read of the PC
      stats.numRegReads++;
      // One mem read, even though it's imem, and there's two of them
      stats.numMemReads++;
      break;
    case ADD_SP:
      // needs stats
      //cout << "This shit is an ADD_SP not an SP_ADD" << endl;
      decode(addsp);
      rf.write(addsp.instr.add.rd, SP + (addsp.instr.add.imm*4));
      //Stats
      stats.numRegWrites += 1;
      stats.numRegReads += 1;
      stats.instrs++;


      break;
    default:
      cout << "[ERROR] Unknown Instruction to be executed" << endl;
      exit(1);
      break;
  }

}
