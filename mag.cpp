#include "mag.h"

using namespace vbit;

Mag::Mag(int mag, std::list<TTXPageStream>* pageSet) :
    _pageSet(pageSet), _page(NULL), _magNumber(mag), _headerFlag(false)
{
    //ctor
    if (_pageSet->size()>0)
    {
        //std::cerr << "[Mag::Mag] enters. page size=" << _pageSet->size() << std::endl;
        _it=_pageSet->begin();
        //_it->DebugDump();
        _page=&*_it;
    }
    _carousel=new vbit::Carousel();
    //std::cerr << "[Mag::Mag] exits" << std::endl;
}

Mag::~Mag()
{
    //std::cerr << "[Mag::Mag] ~ " << std::endl;
    delete _carousel;
    //dtor
}

int Mag::GetPageCount()
{
    return _pageSet->size();
}

/** The next page in a carousel */
TTXPageStream* Mag::GetCarouselPage()
{
    TTXPageStream* p=_carousel->nextCarousel();
    /*
    if (p==NULL)
    {
        std::cerr << "There are no carousels ready to go at the moment" << std::endl;
    }
    */
    return p; // @todo Look at the carousel list and see if one is ready go
}

Packet* Mag::GetPacket()
{
    int thisPage;
		int thisSubcode;
		int thisMag;
		Packet* p=NULL;
		int thisStatus;		
		enum State {STATE_HEADER, STATE_FASTEXT, STATE_PACKET26, STATE_PACKET27, STATE_PACKET28, STATE_TEXTROW};
		static State state=STATE_HEADER;
		
		static vbit::Packet* empty=new Packet();
    // Returns one packet at a time from a page.
    // We enter with _CurrentPage pointing to the first page
    // std::cerr << "[Mag::GetPacket] called " << std::endl;

    // So what page will we send?
    // Pages are two types: single pages and carousels.
    // We won't implement non-carousel page timing. (ie. to make the index page appear more often)
    // The page selection algorithm will then be:
    /* 1) Every page starts off as non-carousel. A flag indicates this state.
     * 2) The page list is iterated through. If a page set has only one page then output that page.
     * 3) If it is flagged as a single page but has multiple pages, add it to a carousel list and flag as multiple.
     * 4) If it is flagged as a multiple page and has multiple pages then ignore it. Skip to the next single page.
     * 5) If it is flagged as a multiple page but only has one page, then delete it from the carousels list and flag as single page.
     * 6) However, before iterating in step 2, do this every second: Look at the carousel list and for each page decrement their timers.
     * When a page reaches 0 then it is taken as the next page, and its timer reset.
     */

     // If there are no pages, we don't have anything. @todo We could go quiet or filler.
    // std::cerr << "[GetPacket] PageSize " << _pageSet->size() << std::endl;
/*
        vbit::Mag* m=_mag[i];
        std::list<TTXPageStream> p=m->Get_pageSet();
        for (std::list<TTXPageStream>::iterator it=p.begin();it!=p.end();++it)
*/
    // If there is no page, we should send a filler?
    if (_pageSet->size()<1)
        return empty; // @todo make this a filler (or quiet or NULL)
				// @todo This looks like it might be a memory leak! Maybe keep a quiet packet handy instead of doing new? 


    //std::cerr << "[GetPacket] DEBUG DUMP 1 " << std::endl;
    //_page->DebugDump();

    TTXLine* txt;
		
		
		switch (state)
		{
		case STATE_HEADER: // Decide which page goes next
        _page=GetCarouselPage(); // Is there a carousel page due?

        if (_page) // Carousel? Step to the next subpage
				{
					_page->StepNextSubpage();
				}
				else  // No carousel? Take the next page in the main sequence
        {
            ++_it;
            if (_it==_pageSet->end())
            {
                _it=_pageSet->begin();
            }
            // Get pointer to the page we are sending
            _page=&*_it;
        }

				// When a single page is changed into a carousel
        if (_page->IsCarousel() != _page->GetCarouselFlag())
        {
            _page->SetCarouselFlag(_page->IsCarousel());
            if (_page->IsCarousel())
            {
                // std::cerr << "This page has become a carousel. Add it to the list" << std::endl;
                _carousel->addPage(_page);
            }
            else
            {
                std::cerr << "This page has no longer a carousel. Remove it from the list" << std::endl;
                exit(3); //
            }
        }

				
        //std::cerr << "[GetPacket] Need to create header packet now " << std::endl;
        // Assemble the header. (we can simplify this or leave it for the optimiser)
        thisPage=_page->GetPageNumber();
				thisPage=(thisPage/0x100) % 0x100; // Remove this line for Continuous Random Acquisition of Pages.				
        thisSubcode=_page->GetSubCode();
        thisStatus=_page->GetPageStatus();
        p=new Packet();
        p->Header(_magNumber,thisPage,thisSubcode,thisStatus);// loads of stuff to do here!

      //p->HeaderText("CEEFAX 1 MPP DAY DD MTH 12:34.56"); // Placeholder 32 characters. This gets replaced later
        p->HeaderText("CEEFAX 1 %%# %%a %d %%b 12:34.56"); // Placeholder 32 characters. This gets replaced later



        p->Parity(13);
				state=STATE_FASTEXT;
				break;
				
		case STATE_FASTEXT:
			std::cerr << "Need to implement Fastext";
			state=STATE_PACKET26;
			break;
		case STATE_PACKET26:
			std::cerr << "Need to implement Packet26";
			state=STATE_PACKET27;
			break;
		case STATE_PACKET27:
			std::cerr << "Need to implement packet27";
			state=STATE_PACKET28;
			break;
		case STATE_PACKET28:
			std::cerr << "Need to implement Packet29";
			state=STATE_HEADER;
			break;
		case STATE_TEXTROW:
			txt=_page->GetNextRow();
			if (txt==NULL)
				state=STATE_HEADER;
			else
			{
				if (txt->GetLine().empty())
					p=NULL;
				// @todo Terminating condition

				// Assemble the packet
				thisMag=_magNumber;
				int thisRow=_page->GetLineCounter(); // The number of the last row received

				Packet* p=new Packet(thisMag, thisRow, txt->GetLine());
				p->Parity();	

				_headerFlag=txt==NULL; // if last line was read
			}
			
			break;
		} // case

//    Dump whole page list for debug purposes
/*
    for (_it=thing1;_it!=thing2;++_it)
    {
        std::cerr << "[Mag::GetPacket]Filename =" << _it->GetSourcePage() << std::endl;
    }
*/



    return p; /// @todo place holder. Need to implement
}

