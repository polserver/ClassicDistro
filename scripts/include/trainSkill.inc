use uo;
use os;
use attributes;

include "include/skillLists";
include ":attributes:attributes";

var cost;

// This determines the max amount an npc can train
// in any given skill from their skill lists defined in
// the function GetSkillList found in skillLists.inc...
//
// Because we are dealing with the attributes system and everything
// needs to be in in the tenths format, this value should be 100 if
// the train cap is 10.0.
Const MAX_VENDOR_TRAIN := 100; // default: 100 (can train up to 10.0 in a given skill)

function VendorTrain( byref me, byref mobile, byref text )

  var words := SplitWords( text );
  if( words.Size() == 2 )
    ListTrainableSkills( me, mobile );
  else
    var skill := words[3];
    if( words[4] )
      skill += " "+words[4];
    endif
    SelectSkill( me, mobile, Lower( skill ) );
  endif
endfunction

function ListTrainableSkills( byref me, byref mobile )

  var train_list := "";
  foreach skill in ( GetSkillList( me ) )
    if( GetAttributeBaseValue( mobile, AP_SkillIdToAttributeName( FindSkillId( skill ) ) ) >= MAX_VENDOR_TRAIN )
      continue;
    endif

    if( !train_list.Length() )
      train_list += skill;
    else
      train_list += ", "+skill;
    endif
  endforeach

  if( !train_list.Length() )
    Say( "Alas, I cannot teach thee anything." );
    return 0;
  endif

  Say( "I can train the following:" );
  Say( train_list );

  return 1;
endfunction

function SelectSkill( byref me, byref mobile, byref skill )

  var trainable_skills := GetSkillList( me ),
    attribute := AP_SkillIdToAttributeName( FindSkillId( skill ) );
  if( !( skill in trainable_skills ) )
    Say( "'Tis not something I can teach thee of." );
    return 0;
  elseif( GetAttributeBaseValue( mobile, attribute ) >= MAX_VENDOR_TRAIN )
    Say( "I cannot teach thee, for thou knowest all I can teach!" );
    return 0;
  endif

  var diff := MAX_VENDOR_TRAIN - GetAttributeBaseValue( mobile, attribute );
  Say( "I will teach thee all I know, if paid the amount in full.  The price is:"+diff );
  SleepMS( 500 );
  Say( "For less I shall teach thee less." );
  SetObjProperty( mobile, "#VendorTrain", Struct{ "Attribute":=attribute, "Cost":=diff, "Serial":=me.serial, "Time":=ReadGameClock()+120 } );
endfunction

function Train( byref me, byref mobile, byref item )

  var train_info := GetObjProperty( mobile, "#VendorTrain" );
  if( train_info.errortext )
    Say( "I think you need this more than I do friend." );
    MoveItemToContainer( item, mobile.backpack );
    return 0;
  elseif( train_info.Serial != me.serial )
    Say( "You must have me mistaken for someone else." );
    MoveItemToContainer( item, mobile.backpack );
    return 0;
  elseif( train_info.Time < ReadGameClock() )
    Say( "Alas, I have forgotten our last discussion." );
    MoveItemToContainer( item, mobile.backpack );
    EraseObjProperty( mobile, "#VendorTrain" );
    return 0;
  endif

  var paid := 0;
  if( item.amount > train_info.Cost )
    SleepMS( 500 );
    SubtractAmount( item, train_info.Cost );
    Say( "You have overpaid me, here is the difference." );
    MoveItemToContainer( item, mobile.backpack );
    EraseObjProperty( mobile, "#VendorTrain" );
    paid := train_info.Cost;
  elseif( item.amount < train_info.Cost )
    paid := item.amount;
    SetObjProperty( mobile, "#VendorTrain", Struct{ "Attribute":=train_info.Attribute, "Cost":=train_info.Cost-item.amount, "Serial":=me.serial, "Time":=ReadGameClock()+120 } );
    DestroyItem( item );
  else
    paid := train_info.Cost;
    EraseObjProperty( mobile, "#VendorTrain" );
    DestroyItem( item );
  endif

  Say( "Let me show thee something of how this is done." );
  SleepMS( 500 );
  SetAttributeBaseValue( mobile, train_info.Attribute, GetAttributeBaseValue( mobile, train_info.Attribute )+paid );

  return 1;
endfunction

function TrainSkill(me, who, item)
  if((item.objtype == 0x1da0) && ((me.npctemplate == "townguard") || (me.npctemplate == "orderguard") || (me.npctemplate == "chaosguard")))
    var parms := array;
    parms.append(me);
    parms.append(who);
    parms.append(item);
    MoveObjectToLocation(item, 5852, 1161, 0, "britannia_alt", MOVEOBJECT_FORCELOCATION);
    item.movable := 0;
    ReleaseItem(item);
    start_script(":bountyboard:collectBounty", parms);
    return;
  endif
  if((item.objtype == 0xeed) && (GetObjProperty(who, "Hire") == me.serial))
    if(GetObjProperty(who, "#arena"))
      say("Im not here for a job!  I'm here to fight!");
      return;
    else
      hireme(me, who, item);
      return;
    endif
  endif
  if(GetObjProperty(who, "#arena"))
    say("You cannot train in an arena");
    return;
  endif
  var variance := 0;
  var npc_level := 0;
  var pc_level := 0;
  var goldreceived := 0;
  var new_level := 0;
  var currentnpccost := 0;
  if((item.objtype == 0xeed) && (item.amount != 0))
    Train( me, who, item );
    // Old Code...
    /*
    var skillid := GetObjProperty(who, "TrainSkillID");
    var trainmaxcost := GetObjProperty(who, "TrainMaxCost");
    var attributeid := AP_SkillIdToAttributeName(GetObjProperty(who, "TrainSkillID"));
    if((!trainmaxcost || !attributeid) && attributeid != 0)
      say("I don't know what this is for, but thanks!");
      DestroyItem(item);
      return;
    endif
    npc_level    := GetAttribute(me, attributeid);
    pc_level     := GetAttributeBaseValue(who, attributeid) / 10;
    goldreceived := item.amount;
    variance     := npc_level - pc_level;
    currentnpccost := GoldForSkillGain(npc_level, pc_level);
    if(pc_level < npc_level)
      if(currentnpccost != trainmaxcost)
        if(currentnpccost > trainmaxcost)
          npc_level := SkillGainForGold(pc_level, trainmaxcost);
          variance  := npc_level - pc_level;
        else
          if(goldreceived > currentnpccost)
            say("I can not train thee to that level.");
            MoveItemToContainer(who.backpack, item);
            return;
          else
            trainmaxcost := currentnpccost;
          endif
        endif
      endif
      if(trainmaxcost <= goldreceived)
        new_level := (npc_level * 10);
        EraseObjProperty( who, "TrainMaxCost" );
        EraseObjProperty( who, "TrainSkillID" );
      else
        new_level := CInt((pc_level + (CDbl(variance) * (CDbl(goldreceived) / CDbl(trainmaxcost)))) * 10.0);
        SetObjProperty( who, "TrainMaxCost", trainmaxcost-goldreceived );
      endif

      PrintTextAbovePrivate(me, "Let me show you how it's done.", who);
	  SetAttributeBaseValue(who, attributeid, new_level);
      DoRecalcVitals(who);
      DestroyItem(item);
      SendSysMessage(who, "Your skill increases.");

    else
      say("I'm not as good as thee.  Please find another to train thee.");
      MoveItemToContainer(who.backpack, item);
    endif
    */
  else
    say("Bah, I have no use for this. Be off with ye.");
    MoveItemToContainer(item, who.backpack);
  endif
endfunction

function hireme(me, who, item)
  var amt := item.amount;
  if(amt < cost)
    say("I will not work for any less than " + cost + " gold coins per day.");
    MoveItemToContainer(who.backpack, item);
  else
    EraseObjProperty(who, "Hire");
    var days := amt / cost;
    say("I will work for you for " + days + " days.");
    DestroyItem(item);
    SetObjProperty(me, "master", who.serial);
    SetObjProperty(me,"script", me.script);
    days := days * 10800;
    SetObjProperty(me, "Timer", (ReadGameClock() + days));
    me.script := "employed";
    RestartScript(me);
    return;
  endif
endfunction

// Old Code...
/*
function MerchantTrain(me, who, text)

  // If player's skill exceeds this number, the NPC won't train it
  var maxskilltotrain := 10;

  if(GetObjProperty(who, "#arena"))
    say("You cannot train in an arena");
    return;
  endif
  var totaltrain := 0;
  var words := SplitWords(text);
  var skill := words[3];
  if(words[4])
    skill := skill + " " + words[4];
  endif
  if(!skill)
    var trainable := "";
    var holder := "";
    foreach thing in getskilllist(me)
      if(holder == "")
        holder := thing;
      else
        holder := holder + ", " + thing;
      endif
      totaltrain := totaltrain + 1;
    endforeach
    trainable  := trainable + holder;
    if(totaltrain > 0)
      say("I can train thee in the following skills:");
      say(trainable);
    else
      say("I can not train thee.");
    endif
  else
    skill := lower(skill);
    foreach thing in getskilllist(me)
      if(lower(thing) == skill)
        var holder := FindSkillId(thing);

// Since so many npcdesc.cfg entries have NPC's with 100 skill, set it lower
// for the purpose of training.

        var npclevel := AP_GetSkill(me, AP_SkillIDToAttributeName(holder));
        if(npclevel == 0)
          npclevel := RandomInt(12) + 22;
          SetBaseSkill(me, holder, npclevel);
          var npclevel := AP_GetSkill(me, AP_SkillIDToAttributeName(holder));

        endif
        if(npclevel > maxskilltotrain)
          npclevel := maxskilltotrain;
        endif
        var pclevel := GetBaseSkill(who, holder);
        if((pclevel >= npclevel) || (pclevel > maxskilltotrain))
          PrintTextAbovePrivate(me, "I cannot teach you anymore.", who);
          return;
        else
          var trainmaxcost := GoldForSkillGain(npclevel, pclevel);
          PrintTextAbovePrivate(me, "I will train thee " + thing + " for " + trainmaxcost + " gold.", who);
          SetObjProperty(who, "TrainMaxCost", trainmaxcost);
          SetObjProperty(who, "TrainSkillID", holder);
          return;
        endif
      endif
    endforeach
    PrintTextAbovePrivate(me, "I know not of that skill.", who);
  endif
endfunction

function GoldForSkillGain(npc_skill, pc_skill)
  return CDbl((npc_skill - pc_skill) * 10.0);
endfunction

function SkillGainForGold(pc_level, goldreceived)
  return (goldreceived / 10) + pc_level;
endfunction
*/