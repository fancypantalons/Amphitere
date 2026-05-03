package com.tbd.NetHack;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;
import android.util.Log;

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class SmokeTest {
    private static final String TAG = "NetHackSmokeTest";

    @Test
    public void testLibraryLoading() {
        try {
            Log.i(TAG, "Attempting to load nethack library...");
            System.loadLibrary("nethack");
            Log.i(TAG, "Successfully loaded nethack library.");
            assertTrue(true);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "FAILED to load nethack library: " + e.getMessage());
            fail("Library load failed: " + e.getMessage());
        }
    }

    @Test
    public void testNativeMethodLinkage() {
        try {
            System.loadLibrary("nethack");
            Class<?> ioClass = Class.forName("com.tbd.forkfront.NetHackIO");
            boolean hasRunMethod = false;
            for (java.lang.reflect.Method m : ioClass.getDeclaredMethods()) {
                if (m.getName().equals("RunNetHack")) {
                    hasRunMethod = true;
                    break;
                }
            }
            assertTrue("RunNetHack native method should be defined in NetHackIO", hasRunMethod);
        } catch (Exception | UnsatisfiedLinkError e) {
            fail("Linkage check failed: " + e.getMessage());
        }
    }
}
