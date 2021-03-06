import utime

## Version
### First didget = Software type 1-Production 2-Beta 3-Alpha
### Secound and third didget = Major version number
### Fourth to sixth = Minor version number
Version = 300001

class Init:


    # ++++++++++++++++++++++++++++++++++++++++++++++++++ MISC ++++++++++++++++++++++++++++++++++++++++++++++++++

    # Custom Exception
    class Timer_Error(Exception):
        pass

    # -------------------------------------------------------------------------------------------------------
    def __init__(self, Dobby):
        # Referance to dobby
        self.Dobby = Dobby
        self.Dobby.Log(1, "Timer", "Initializing")
        # Var to hold configured Timeres
        self.Timers = {}
        # Log Event
        self.Dobby.Log(0, "Timer", "Initialization complete")


    # -------------------------------------------------------------------------------------------------------
    def Loop(self):
        # Run loop for all Timers
        for Name in self.Timers:
            if self.Timers[Name].Delete == True:
                self.Dobby.Log(0, "Timer", "Deleting: " + str(Name))
                del self.Timers[Name]
            else:
                # Run loop
                self.Timers[Name].Loop()


    # -------------------------------------------------------------------------------------------------------
    def Add(self, Name, Timeout_ms, Callback, Argument=None, Logging=True, Start=False, Keep=True, Repeat=False):

        # Check if timer already exists
        if self.Timers.get(Name, None) != None:
            raise self.Timer_Error("Timer name: '" + str(Name) + "' already in use")

        # Convert Timeout_ms if given as string
        Timeout_ms = self.Time_To_ms(Timeout_ms)

        # Create Timer
        self.Timers[Name] = self.New_Timer(self.Dobby, Name, Timeout_ms, Callback, Argument, Logging, Start, Keep, Repeat)
        # Return timer so we can use the as refferance in the other end
        return self.Timers[Name]


    # -------------------------------------------------------------------------------------------------------
    def _To_ms(self, Time):

        if type(Time) != str:
            return Time

        try:
            if Time.lower().endswith("s") == True:
                # Convert Time to ms
                return int(float(Time[:-1]) * 1000)
                
            elif Time.lower().endswith("m") == True:
                # Convert Time to ms
                return int(float(Time[:-1]) * 60000)
                
            elif Time.lower().endswith("h") == True:
                # Convert Time to ms
                return int(float(Time[:-1]) * 3600000)

        except ValueError:
            # Raise error
            raise self.Timer_Error("Invalid time provided: " + str(Time))

    # -------------------------------------------------------------------------------------------------------
    def Time_To_ms(self, Time, Min_Value=None):
        # if Time is int will return it assuming it already in ms format
        # Converts a string contrining <number of><format> to ms
        # Format options: 
        #     s = seconds
        #     m = minutes
        #     h = hours
        # Check if int with try and return it assuming its ms already
        try: 
            return int(Time)
        except ValueError:
            pass

        # we need at least one number and one char
        if len(Time) < 2:
            # pass so we trigger error
            pass

        # Convert Time and Min value to ms
        Time_ms = self._To_ms(Time)

        if Min_Value != None:
            Min_ms = self._To_ms(Min_Value)
            # Compare the two values and default to Min_ms if Time_ms is lower then Min_ms
            if Time_ms < Min_ms:
                # Default to Min_ms aka Min_Value
                Time_ms = Min_ms
                # Log event
                self.Dobby.Log(2, "Timer", "Time: " + str(Time) + " is less then Min Value: " + str(Min_Value) + " defaulting to Min Value")

        # Return the time in ms we made
        return Time_ms


    class New_Timer:
        # -------------------------------------------------------------------------------------------------------
        def __init__(self, Dobby, Name, Timeout_ms, Callback, Argument, Logging, Start, Keep, Repeat):
            # Referance to dobby
            self.Dobby = Dobby
            # Log event
            self.Dobby.Log(0, "Timer", "Initializing: " + str(Name))

            # Store name
            self.Name = Name
            # Store logging
            self.Logging = Logging
            # How long before triggering the command
            self.Timeout_ms = Timeout_ms
            # This var is the one we use diff() on and then check agains Timeout_ms
            # if larger then timeout ms then trigger action
            self.Start_ms = utime.ticks_ms

            # If set to True, this class instance will be deleted during next run of Timer.Loop()
            self.Delete = False

            # What function the timer is going to trigger
            self.Callback = Callback
            # Argument to trigger with command
            self.Argument = Argument

            # State of the timer:
            # True = Running
            # False = Ideling / Waiting for reset
            self.Running = False

            self.Keep = Keep

            self.Repeat = Repeat

            # Start timer if requested
            if Start == True:
                self.Start()


        # -------------------------------------------------------------------------------------------------------
        def Loop(self):
            # Check if we are running
            if self.Running == False:
                return

            # Trigger the callback with argument
            if utime.ticks_diff(utime.ticks_ms(), self.Start_ms) < self.Timeout_ms:
                return
            try:
                if utime.ticks_diff(utime.ticks_ms(), self.Start_ms) < self.Timeout_ms:
                    return
            except TypeError as e:
                raise self.Dobby.Sys_Modules['timer'].Timer_Error("Error: " + str(e) + " Name:" + str(self.Name) + " Timeout_ms:" + str(self.Timeout_ms) + " Callback:" + str(self.Callback) + " Argument:" + str(self.Argument))

            if self.Logging == True:
                # Log event
                self.Dobby.Log(0, "Timer", "Triggering callback: " + self.Name)

            if self.Argument != None:
                # Trigger the callback with argument
                # try:
                self.Callback(self.Argument)
                # except TypeError as e:
                #     raise self.Dobby.Sys_Modules['timer'].Timer_Error("Error running callback:" + str(self.Name) + " Argument:" + str(self.Argument) + " Error:" + str(e))
            else:
                # Trigger the callback without arguemtn
                self.Callback()

            # if keep is false, set delete = true so the timer gets deleted by next iteration of main loop (above loop)
            if self.Keep == False:
                self.Delete = True

            # if Repeat it set to true restart the timer
            elif self.Repeat == True:
                self.Start()


        # -------------------------------------------------------------------------------------------------------
        def Set_Timeout(self, Timeout):
            self.Timeout_ms = self.Dobby.Sys_Modules['timer'].Time_To_ms(Timeout)
            

        # -------------------------------------------------------------------------------------------------------
        def Start(self):
            # Mark the timer as running
            self.Running = True
            # Note or reset start time
            self.Start_ms = utime.ticks_ms()


        # -------------------------------------------------------------------------------------------------------
        def Stop(self, Delete=False):
            # Do nothing if already stopped
            if self.Running == False:
                return 
            # Stopps the timer if active
            self.Running = False
            # Check if we need to mark the timer for deleation in next Timer.Loop
            if Delete == True or self.Keep == False:
                self.Delete = True