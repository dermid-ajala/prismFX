function prismfx_init(_this) {
	_this.appendDummyInput()
		.appendField(Blockly.Msg.PRISMFX_TITLE);

	// device addess 0x20 to 0x23 for channel 0, addess 0x20 to 0x27 for channel 1 to 64
	_this.appendDummyInput()
		.appendField(Blockly.Msg.ADDRESS)
		.appendField(new Blockly.FieldDropdown(function() {
			try {
				if ((typeof(this.sourceBlock_) != "undefined") && (typeof(this.sourceBlock_.inputList) != "undefined")) {
					var inputlist = this.sourceBlock_.inputList;
					var selected_channel = parseInt(inputlist[1].fieldRow[1].value_);
					return Blockly.mcp23s17_address_dropdown_menu(selected_channel);
				}
			} catch (e) {

			}
			// default
			return Blockly.mcp23s17_address_dropdown_menu(0);
		}), 'ADDRESS');
}

Blockly.Blocks["prismfx.clear"] = {
	init: function() {
		// init
		prismfx_init(this);
                this.appendDummyInput().appendField("CLEAR");
                this.appendDummyInput().appendField("Rotation")
					.appendField(new Blockly.FieldDropdown([
					["0 Pins on top"   , "0"],
					["1 Pins on left"  , "1"],
					["2 Pins on bottom", "2"],
					["3 Pins on right" , "3"]
					]), "Rotation");
		this.appendValueInput("COLOR").setCheck("Number").appendField("COLOR");
		this.setInputsInline(true);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setColour(160);
		this.setTooltip("");
		this.setHelpUrl("");
}};

Blockly.Blocks["prismfx.setTextColor"] = {
	init: function() {
		prismfx_init(this);
		this.appendDummyInput().appendField(Blockly.Msg.PRISMFX_TEXT_COLOR_TITLE);
		this.appendValueInput("Fore").setCheck("Number").appendField("ForeColor");
		this.appendValueInput("Back").setCheck("Number").appendField("BackColor");
		this.setInputsInline(true);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setColour(160);
		this.setTooltip(Blockly.Msg.PRISMFX_TEXT_COLOR_TOOLTIP);
		this.setHelpUrl(Blockly.Msg.PRISMFX_TEXT_COLOR_HELPURL);
}};

Blockly.Blocks["prismfx.print"] = {
	init: function() {
		prismfx_init(this);
		this.appendDummyInput().appendField(Blockly.Msg.PRISMFX_PRINT_TITLE);
		this.appendValueInput("Col").setCheck("Number" ).appendField("Column");
		this.appendValueInput("Row").setCheck("Number" ).appendField("Row"   );
		this.appendValueInput('Str').setCheck("String" ).appendField("Str"   );
		this.appendDummyInput().appendField("Size").appendField(new Blockly.FieldDropdown(
			[ [ "S Small 6x8"   , "0" ], [ "M Medium 10x15" , "1" ] ]), "Size");
		this.setInputsInline(true);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setColour(160);
		this.setTooltip(Blockly.Msg.PRISMFX_PRINT_TOOLTIP);
		this.setHelpUrl(Blockly.Msg.PRISMFX_PRINT_HELPURL);
}};

Blockly.Blocks["prismfx.printGFX"] = {
	init: function() {
		prismfx_init(this);
		this.appendDummyInput().appendField("Print GFX");
		this.appendValueInput("Col").setCheck("Number" ).appendField("Column");
		this.appendValueInput("Row").setCheck("Number" ).appendField("Row"   );
		this.appendValueInput('Str').setCheck("String" ).appendField("Str TH&EN"   );
		this.setInputsInline(true);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setColour(160);
		this.setTooltip("");
		this.setHelpUrl("");
}};

Blockly.Blocks['prismfx.point'] = {
	init: function() {
	  prismfx_init(this);
	  this.appendDummyInput().appendField("Point");
	  this.appendValueInput("X1").setCheck("Number").appendField("X1");
	  this.appendValueInput("Y1").setCheck("Number").appendField("Y1");
	  this.appendValueInput("COLOR").setCheck("Number").appendField("COLOR");
	  this.setInputsInline(true);
	  this.setPreviousStatement(true, null);
	  this.setNextStatement(true, null);
	  this.setColour(150);
	  this.setTooltip("");
	  this.setHelpUrl("");
  }};
  
  Blockly.Blocks['prismfx.line'] = {
	init: function() {
	  prismfx_init(this);
	  this.appendDummyInput().appendField("Line");
	  this.appendValueInput("X1").setCheck("Number").appendField("X1");
	  this.appendValueInput("Y1").setCheck("Number").appendField("Y1");
	  this.appendValueInput("X2").setCheck("Number").appendField("X2");
	  this.appendValueInput("Y2").setCheck("Number").appendField("Y2");
	  this.appendValueInput("COLOR").setCheck("Number").appendField("COLOR");
	  this.setInputsInline(true);
	  this.setPreviousStatement(true, null);
	  this.setNextStatement(true, null);
	  this.setColour(150);
	  this.setTooltip("");
	  this.setHelpUrl("");
  }};
  
  Blockly.Blocks['prismfx.rectangle'] = {
	init: function() {
	  prismfx_init(this);
	  this.appendDummyInput().appendField("Rectangle");
	  this.appendValueInput("X1").setCheck("Number").appendField("X1");
	  this.appendValueInput("Y1").setCheck("Number").appendField("Y1");
	  this.appendValueInput("X2").setCheck("Number").appendField("X2");
	  this.appendValueInput("Y2").setCheck("Number").appendField("Y2");
	  this.appendValueInput("COLOR").setCheck("Number").appendField("COLOR");
	  this.appendDummyInput().appendField("Fill").appendField(new Blockly.FieldDropdown(
		[ [ "Filled", "true" ], [ "Outline", "false" ] ]), "Fill");
	  this.setInputsInline(true);
	  this.setPreviousStatement(true, null);
	  this.setNextStatement(true, null);
	  this.setColour(150);
	  this.setTooltip("");
	  this.setHelpUrl("");
  }};

  Blockly.Blocks['prismfx.triangle'] = {
	init: function() {
	  prismfx_init(this);
	  this.appendDummyInput().appendField("Triangle");
	  this.appendValueInput("X1").setCheck("Number").appendField("X1");
	  this.appendValueInput("Y1").setCheck("Number").appendField("Y1");
	  this.appendValueInput("X2").setCheck("Number").appendField("X2");
	  this.appendValueInput("Y2").setCheck("Number").appendField("Y2");
	  this.appendValueInput("X3").setCheck("Number").appendField("X3");
	  this.appendValueInput("Y3").setCheck("Number").appendField("Y3");
	  this.appendValueInput("COLOR").setCheck("Number").appendField("COLOR");
	  this.appendDummyInput().appendField("Fill").appendField(new Blockly.FieldDropdown(
		[ [ "Filled", "true" ], [ "Outline", "false" ] ]), "Fill");
	  this.setInputsInline(true);
	  this.setPreviousStatement(true, null);
	  this.setNextStatement(true, null);
	  this.setColour(150);
	  this.setTooltip("");
	  this.setHelpUrl("");
  }};
  
Blockly.Blocks['prismfx.picker'] = {
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldColour("#f70000"), "COLOR");
    this.setOutput(true, "Number");
    this.setColour(230);
 this.setTooltip("");
 this.setHelpUrl("");
}};


Blockly.Blocks['prismfx.num2str'] = {
	init: function() {
		this.appendDummyInput().appendField(Blockly.Msg.PRISMFX_NUM2STR_TITLE);
		//read this in generator V       type check V                     V display this in block
		this.appendValueInput("Value" ).setCheck("Number").appendField("Value" );
		this.appendValueInput("Width" ).setCheck("Number").appendField("Width" );
		this.appendDummyInput().appendField("Decimals")
	 		.appendField(new Blockly.FieldDropdown([
	 			["0", "0"],
				["1", "1"],
	  			["2", "2"],
	  			["3", "3"],
	  			["4", "4"],
	  			["5", "5"],
	  			["6", "6"]
				]), "Decimals");
		this.appendDummyInput().appendField("Format")
			.appendField(new Blockly.FieldDropdown([
				["integer"         , "0"],
				["hexadecimal"     , "1"],
				["hex w/leading 0s", "2"],
				["fixed point"     , "3"],
				["exponential"     , "4"]
			   ]), "Format");
		this.setInputsInline(true);
		this.setOutput(true, "String");
		this.setColour(160);
		this.setTooltip(Blockly.Msg.PRISMFX_NUM2STR_TOOLTIP);
		this.setHelpUrl('');
}};

Blockly.Blocks["prismfx.initplot"] = {
	init: function() {
		// init
		prismfx_init(this);
        this.appendDummyInput().appendField("init plot");
        this.appendDummyInput().appendField("index").appendField(new Blockly.FieldDropdown([	// <-- name displayed in block
				["1", "0"],
				["2", "1"],
				["3", "2"]]), "inx");	// << name for generator
		this.appendValueInput("nam").setCheck("String").appendField("variable name");
		this.appendValueInput("unt").setCheck("String").appendField("units");
		this.appendValueInput("min").setCheck("Number").appendField("plotMin");
		this.appendValueInput("max").setCheck("Number").appendField("plotMax");
		this.appendDummyInput().appendField("decimals").appendField(new Blockly.FieldDropdown([	// <-- name displayed in block
				["0", "0"],
				["1", "1"],
				["2", "2"],
				["3", "3"],
				["4", "4"]]), "dec");	// << name for generator
		this.appendValueInput("col").setCheck("Number").appendField("color");
		this.setInputsInline(true);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setColour(160);
		this.setTooltip("");
		this.setHelpUrl("");
}};

Blockly.Blocks["prismfx.plotdata"] = {
	init: function() {
		// init
		prismfx_init(this);
        this.appendDummyInput().appendField("iplot data");
		this.appendValueInput("v1").setCheck("Number").appendField("var1");
		this.appendValueInput("v2").setCheck("Number").appendField("var2");
		this.appendValueInput("v3").setCheck("Number").appendField("var3");
		this.setInputsInline(true);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setColour(160);
		this.setTooltip("");
		this.setHelpUrl("");
}};
